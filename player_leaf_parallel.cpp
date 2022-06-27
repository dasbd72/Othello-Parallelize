#include <omp.h>

#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <limits>
#include <list>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

// typedef std::pair<int, int> Point;
struct Point {
    int x, y;
    Point() : Point(0, 0) {}
    Point(int x, int y) : x(x), y(y) {}
    bool operator==(const Point& rhs) const {
        return x == rhs.x && y == rhs.y;
    }
    bool operator!=(const Point& rhs) const {
        return !operator==(rhs);
    }
    Point operator+(const Point& rhs) const {
        return Point(x + rhs.x, y + rhs.y);
    }
    Point operator-(const Point& rhs) const {
        return Point(x - rhs.x, y - rhs.y);
    }
};

enum SPOT_STATE {
    EMPTY = 0,
    BLACK = 1,
    WHITE = 2
};
const int SIZE = 8;
const int MAXITER = 10000;
int MAX_DURATION = 2000;
const double DIV_DELTA = 1e-9;
const int directions[16] = {-1, -1, -1, 0, -1, 1, 0, -1, 0, 1, 1, -1, 1, 0, 1, 1};
int num_threads = omp_get_max_threads();
std::vector<std::thread> threads(num_threads);
std::mutex lock;
int DEBUG = 0;

struct Node {
    int partial_wins;
    int partial_games;
    int* total_games;

    int player;  // 1: O, 2: X
    int64_t board[2];
    int disc_count[3];
    Node* parent;
    std::vector<std::pair<Node*, Point>> children;

    Node(std::array<std::array<int, SIZE>, SIZE>& board, int player) {
        this->partial_wins = 0;
        this->partial_games = 0;
        this->total_games = &(this->partial_games);

        this->player = player;
        this->board[0] = 0;
        this->board[1] = 0;
        this->disc_count[0] = this->disc_count[1] = this->disc_count[2] = 0;
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                this->disc_count[board[i][j]]++;
                this->board[i >> 2] |= ((int64_t)board[i][j] << ((((i & 3) << 3) + j) << 1));
            }
        }
        this->parent = nullptr;
    }
    Node(Node* node) {
        this->partial_wins = 0;
        this->partial_games = 0;
        this->total_games = node->total_games;

        this->player = node->player;
        this->board[0] = node->board[0];
        this->board[1] = node->board[1];
        this->disc_count[0] = node->disc_count[0];
        this->disc_count[1] = node->disc_count[1];
        this->disc_count[2] = node->disc_count[2];
        this->parent = nullptr;
    }
    ~Node() {
        for (auto child : children) {
            delete child.first;
        }
    }
};

int count_nodes(Node* node);
bool isTerminal(int64_t board[2]);
bool isTerminal(Node* node);

void monte_carlo_tree_search(Node* root, std::chrono::_V2::system_clock::time_point start_time);
void traversal(Node* root, Node*& target);
void expansion(Node* node);
void rollout(unsigned int& seed, Node* node, int& win);
void backPropagation(Node* node, int win);

int main(int argc, char** argv) {
    assert(argc >= 3);
    std::ifstream fin(argv[1]);
    std::ofstream fout(argv[2]);
    int player;
    std::array<std::array<int, SIZE>, SIZE> board;
    if (argc > 3)
        MAX_DURATION = atoi(argv[3]);
    if (argc > 4)
        DEBUG = atoi(argv[4]);
    // === Read state ===
    fin >> player;
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            fin >> board[i][j];
        }
    }
    // === Write move ===

    Node* root = new Node(board, 3 - player);
    auto start_time = std::chrono::high_resolution_clock::now();
    if (!isTerminal(root))
        expansion(root);
    monte_carlo_tree_search(root, start_time);
    auto end_time = std::chrono::high_resolution_clock::now();
    int duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    if (DEBUG >= 1) {
        std::cout << "duration of mcts: " << duration << " milliseconds\n";
        std::cout << "total nodes: " << count_nodes(root) << "\n";
        std::cout << "total simulations: " << root->partial_games << "\n";
        for (size_t i = 0; i < root->children.size(); i++) {
            if (root->children[i].first)
                std::cout << i
                          << " (" << root->children[i].second.x << ", " << root->children[i].second.y << "): "
                          << root->children[i].first->partial_wins << "/" << root->children[i].first->partial_games << " = " << (double)root->children[i].first->partial_wins / (root->children[i].first->partial_games + DIV_DELTA) << "\n";
        }
    }

    Point p = root->children[0].second;
    double max_val = -std::numeric_limits<double>::infinity();
    for (size_t idx = 0; idx < root->children.size(); idx++) {
        double val = (double)root->children[idx].first->partial_wins / (root->children[idx].first->partial_games + DIV_DELTA);
        if (val > max_val) {
            max_val = val;
            p = root->children[idx].second;
        }
    }
    fout << p.x << " " << p.y << std::endl;
    fout.flush();

    /* Node* node = root;
    while (node != nullptr) {
        int selection;
        std::cout << "cur_player: " << node->cur_player << "\n";
        std::cout << "disc_count: " << (int)node->disc_count[0] << ", " << (int)node->disc_count[1] << ", " << (int)node->disc_count[2] << "\n";
        std::cout << node->partial_wins << "/" << node->partial_games << "\n";
        std::cout << "+---------------+\n";
        for (size_t i = 0; i < SIZE; i++) {
            std::cout << "|";
            for (int j = 0; j < SIZE; j++) {
                if (j != 0)
                    std::cout << " ";
                if (((node->board[i >> 2] >> ((((i & 3) << 3) + j) << 1)) & 3) == 1) {
                    std::cout << "O";
                } else if (((node->board[i >> 2] >> ((((i & 3) << 3) + j) << 1)) & 3) == 2) {
                    std::cout << "X";
                } else {
                    std::cout << " ";
                }
            }
            std::cout << "|\n";
        }
        std::cout << "+---------------+\n";
        std::cout << "child size: " << node->children.size() << "\n";
        for (size_t i = 0; i < node->children.size(); i++) {
            if (node->children[i].first)
                std::cout << i
                          << " (" << node->children[i].second.x << ", " << node->children[i].second.y << "): "
                          << node->children[i].first->partial_wins << "/" << node->children[i].first->partial_games << " = " << (double)node->children[i].first->partial_wins / (node->children[i].first->partial_games + DIV_DELTA) << "\n";
        }
        std::cin >> selection;
        if (selection >= node->children.size() || selection < 0)
            node = node->parent;
        else
            node = node->children[selection].first;
    } */

    // === Finalize ===
    fin.close();
    fout.close();
    delete root;
    return 0;
}

int count_nodes(Node* node) {
    if (node == nullptr)
        return 0;
    int val = 0;
    for (auto child : node->children)
        val += count_nodes(child.first) + 1;
    return val;
}

bool isTerminal(int64_t board[2]) {
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (((board[i >> 2] >> ((((i & 3) << 3) + j) << 1)) & 3) == EMPTY) {
                for (int d = 0; d < 8; d++) {
                    Point dir = Point(directions[d << 1], directions[(d << 1) + 1]);
                    Point p = Point(i, j) + dir;
                    if (!(p.x & (~7)) && !(p.y & (~7))) {
                        int firstPly = ((board[p.x >> 2] >> ((((p.x & 3) << 3) + p.y) << 1)) & 3);
                        while (!(p.x & (~7)) && !(p.y & (~7)) && ((board[p.x >> 2] >> ((((p.x & 3) << 3) + p.y) << 1)) & 3) != EMPTY) {
                            if (((board[p.x >> 2] >> ((((p.x & 3) << 3) + p.y) << 1)) & 3) != firstPly)
                                return false;
                            p = p + dir;
                        }
                    }
                }
            }
        }
    }
    return true;
}

bool isTerminal(Node* node) {
    if (node->disc_count[0] == 0)
        return true;
    return isTerminal(node->board);
}

void monte_carlo_tree_search(Node* root, std::chrono::_V2::system_clock::time_point start_time) {
    int iteration = 0;
    unsigned int seed[num_threads];
    Node* curnode = nullptr;
    int win = 0;
    for (int t = 0; t < num_threads; t++) {
        seed[t] = t;
    }
    while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time).count() < MAX_DURATION) {
        iteration++;
        traversal(root, curnode);
        if (!isTerminal(curnode) && curnode->partial_games != 0) {
            expansion(curnode);
            curnode = curnode->children[0].first;
        }
        win = 0;
#pragma omp parallel for schedule(static) num_threads(num_threads)
        for (int t = 0; t < num_threads; t++) {
            rollout(seed[t], curnode, win);
        }
        backPropagation(curnode, win);
    }
    if (DEBUG >= 1)
        std::cout << std::string("iterations: ") + std::to_string(iteration) + std::string("\n");
}

void traversal(Node* root, Node*& target) {
    target = root;
    while (!target->children.empty()) {
        Node* tmpnode = target->children[0].first;
        double max_UCT = -std::numeric_limits<double>::infinity();
        for (auto child : target->children) {
            double tmp = ((double)child.first->partial_wins / (child.first->partial_games + DIV_DELTA)) + sqrt(2 * log(*child.first->total_games) / (child.first->partial_games + DIV_DELTA));
            if (tmp > max_UCT) {
                max_UCT = tmp;
                tmpnode = child.first;
            }
        }
        target = tmpnode;
    }
}

void expansion(Node* node) {
    if (!node->children.empty())
        return;

    if (node->children.empty())
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                if (((node->board[i >> 2] >> ((((i & 3) << 3) + j) << 1)) & 3) == EMPTY) {
                    bool point_availible = false;
                    for (int d = 0; d < 8; d++) {
                        Point dir = Point(directions[d << 1], directions[(d << 1) + 1]);
                        Point p = Point(i, j) + dir;
                        if (!(p.x & (~7)) && !(p.y & (~7))) {
                            if (((node->board[p.x >> 2] >> ((((p.x & 3) << 3) + p.y) << 1)) & 3) != node->player)
                                continue;
                            p = p + dir;
                            while (!(p.x & (~7)) && !(p.y & (~7)) && ((node->board[p.x >> 2] >> ((((p.x & 3) << 3) + p.y) << 1)) & 3) != EMPTY) {
                                if (((node->board[p.x >> 2] >> ((((p.x & 3) << 3) + p.y) << 1)) & 3) == 3 - node->player) {
                                    point_availible = true;
                                    break;
                                }
                                p = p + dir;
                            }
                        }
                    }
                    if (point_availible) {
                        node->children.emplace_back(nullptr, Point(i, j));
                    }
                }
            }
        }

    if (node->children.empty()) {
        Node* child_node = new Node(node);
        child_node->player = 3 - child_node->player;
        child_node->parent = node;
        node->children.emplace_back(child_node, Point(-1, -1));
    } else {
        for (size_t idx = 0; idx < node->children.size(); idx++) {
            if (node->children[idx].first == nullptr) {
                Node* child_node = new Node(node);
                for (int d = 0; d < 8; d++) {
                    Point dir = Point(directions[d << 1], directions[(d << 1) + 1]);
                    Point p = node->children[idx].second + dir;

                    while (!(p.x & (~7)) && !(p.y & (~7)) && ((node->board[p.x >> 2] >> ((((p.x & 3) << 3) + p.y) << 1)) & 3) != EMPTY) {
                        if (((node->board[p.x >> 2] >> ((((p.x & 3) << 3) + p.y) << 1)) & 3) == 3 - node->player) {
                            p = p - dir;
                            while (p != node->children[idx].second) {
                                child_node->disc_count[3 - child_node->player]++;
                                child_node->disc_count[child_node->player]--;
                                child_node->board[p.x >> 2] ^= (int64_t)3 << ((((p.x & 3) << 3) + p.y) << 1);
                                p = p - dir;
                            }
                            break;
                        }
                        p = p + dir;
                    }
                }
                Point p = node->children[idx].second;
                child_node->disc_count[0]--;
                child_node->disc_count[3 - child_node->player]++;
                child_node->board[p.x >> 2] &= ~((int64_t)3 << ((((p.x & 3) << 3) + p.y) << 1));
                child_node->board[p.x >> 2] |= ((int64_t)(3 - child_node->player) << ((((p.x & 3) << 3) + p.y) << 1));
                child_node->player = 3 - child_node->player;
                child_node->parent = node;
                node->children[idx].first = child_node;
            }
        }
    }
}

void rollout(unsigned int& seed, Node* node, int& win) {
    int player = node->player;
    int64_t curboard[2];
    curboard[0] = node->board[0];
    curboard[1] = node->board[1];
    int disc_count[3];
    disc_count[0] = node->disc_count[0];
    disc_count[1] = node->disc_count[1];
    disc_count[2] = node->disc_count[2];
    while (disc_count[0] != 0 && !isTerminal(curboard)) {
        std::vector<Point> next_spots;
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                if (((curboard[i >> 2] >> ((((i & 3) << 3) + j) << 1)) & 3) == EMPTY) {
                    bool point_availible = false;
                    for (int d = 0; d < 8 && !point_availible; d++) {
                        Point dir = Point(directions[d << 1], directions[(d << 1) + 1]);
                        Point p = Point(i, j) + dir;
                        if (!(p.x & (~7)) && !(p.y & (~7))) {
                            if (((curboard[p.x >> 2] >> ((((p.x & 3) << 3) + p.y) << 1)) & 3) != player)
                                continue;
                            p = p + dir;
                            while (!(p.x & (~7)) && !(p.y & (~7)) && ((curboard[p.x >> 2] >> ((((p.x & 3) << 3) + p.y) << 1)) & 3) != EMPTY) {
                                if (((curboard[p.x >> 2] >> ((((p.x & 3) << 3) + p.y) << 1)) & 3) == 3 - player) {
                                    point_availible = true;
                                    break;
                                }
                                p = p + dir;
                            }
                        }
                    }
                    if (point_availible) {
                        next_spots.emplace_back(i, j);
                    }
                }
            }
        }
        if (!next_spots.empty()) {
            size_t idx = rand_r(&seed) % next_spots.size();

            for (int d = 0; d < 8; d++) {
                Point dir = Point(directions[d << 1], directions[(d << 1) + 1]);
                Point p = next_spots[idx] + dir;

                while (!(p.x & (~7)) && !(p.y & (~7)) && ((curboard[p.x >> 2] >> ((((p.x & 3) << 3) + p.y) << 1)) & 3) != EMPTY) {
                    if (((curboard[p.x >> 2] >> ((((p.x & 3) << 3) + p.y) << 1)) & 3) == 3 - player) {
                        p = p - dir;
                        while (p != next_spots[idx]) {
                            disc_count[3 - player]++;
                            disc_count[player]--;
                            curboard[p.x >> 2] ^= (int64_t)3 << ((((p.x & 3) << 3) + p.y) << 1);
                            p = p - dir;
                        }
                        break;
                    }
                    p = p + dir;
                }
            }
            Point p = next_spots[idx];
            disc_count[0]--;
            disc_count[3 - player]++;
            curboard[p.x >> 2] &= ~((int64_t)3 << ((((p.x & 3) << 3) + p.y) << 1));
            curboard[p.x >> 2] |= ((int64_t)(3 - player) << ((((p.x & 3) << 3) + p.y) << 1));
        }
        player = 3 - player;
    }
    int val = disc_count[node->player] - disc_count[3 - node->player];
    if (val > 0) {
#pragma omp critical
        win += 1;
    } else if (val < 0) {
#pragma omp critical
        win -= 1;
    }
}

void backPropagation(Node* node, int win) {
    Node* curnode = node;
    while (curnode != nullptr) {
        curnode->partial_wins += win;
        curnode->partial_games += num_threads;
        curnode = curnode->parent;
        win = -win;
    }
}