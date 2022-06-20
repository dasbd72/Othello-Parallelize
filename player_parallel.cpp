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
const int MAX_DURATION = 2000;
const double DIV_DELTA = 1e-9;
const int directions[16] = {-1, -1, -1, 0, -1, 1, 0, -1, 0, 1, 1, -1, 1, 0, 1, 1};

struct Node {
    int partial_wins;
    int partial_games;
    int* total_games;
    bool isLeaf;

    int cur_player;  // 1: O, 2: X
    int64_t board[2];
    std::array<uint8_t, 3> disc_count;
    Node* parent;
    std::vector<std::pair<Node*, Point>> children;

    Node(std::array<std::array<int, SIZE>, SIZE>& board, int player) {
        this->partial_wins = 0;
        this->partial_games = 0;
        this->total_games = &(this->partial_games);
        this->isLeaf = true;

        this->cur_player = player;
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
        this->isLeaf = true;

        this->cur_player = node->cur_player;
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

bool isTerminal(Node* node);

void monte_carlo_tree_search(Node* root, std::chrono::_V2::system_clock::time_point start_time);
Node* traversal(Node* root);
void expansion(Node* node);
int rollout(Node* node, int player);
void backPropagation(Node* node, int win);

int main(int argc, char** argv) {
    assert(argc == 3);
    srand(time(NULL));
    std::ifstream fin(argv[1]);
    std::ofstream fout(argv[2]);
    int player;
    std::array<std::array<int, SIZE>, SIZE> board;
    // === Read state ===
    fin >> player;
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            fin >> board[i][j];
        }
    }
    // === Write move ===

    Node* root = new Node(board, player);
    auto start_time = std::chrono::high_resolution_clock::now();
    monte_carlo_tree_search(root, start_time);
    auto end_time = std::chrono::high_resolution_clock::now();
    int duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    printf("duration of mcts: %d milliseconds\n", duration);

    Point p;
    double max_val = -std::numeric_limits<double>::max();
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
        std::cout << "isLeaf: " << node->isLeaf << "\n";
        std::cout << "cur_player: " << node->cur_player << "\n";
        std::cout << "disc_count: " << (int)node->disc_count[0] << ", " << (int)node->disc_count[1] << ", " << (int)node->disc_count[2] << "\n";
        std::cout << node->partial_wins << "/" << node->partial_games << "\n";
        // for (int i = 0; i < SIZE; i++) {
        //     for (int j = 0; j < SIZE; j++) {
        //         std::cout << i << ", " << j << ": " << ((((i & 3) << 3) + j) << 1) << " " << ((node->board[i >> 2] >> ((((i & 3) << 3) + j) << 1)) & 3) << "\n";
        //     }
        // }
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

bool isTerminal(Node* node) {
    if (node->disc_count[0] != 0) {
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                if (((node->board[i >> 2] >> ((((i & 3) << 3) + j) << 1)) & 3) == EMPTY) {
                    for (int d = 0; d < 8; d++) {
                        Point dir = Point(directions[d << 1], directions[(d << 1) + 1]);
                        Point p = Point(i, j) + dir;
                        if (!(p.x & (~7)) && !(p.y & (~7))) {
                            int firstPly = ((node->board[p.x >> 2] >> ((((p.x & 3) << 3) + p.y) << 1)) & 3);
                            while (!(p.x & (~7)) && !(p.y & (~7)) && ((node->board[p.x >> 2] >> ((((p.x & 3) << 3) + p.y) << 1)) & 3) != EMPTY) {
                                if (((node->board[p.x >> 2] >> ((((p.x & 3) << 3) + p.y) << 1)) & 3) != firstPly)
                                    return false;
                                p = p + dir;
                            }
                        }
                    }
                }
            }
        }
    }
    return true;
}

void monte_carlo_tree_search(Node* root, std::chrono::_V2::system_clock::time_point start_time) {
    int iteration = 0;
    while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time).count() < MAX_DURATION) {
        iteration++;
        Node* curnode;
        curnode = traversal(root);
        if (!isTerminal(curnode) && curnode->partial_games != 0) {
            expansion(curnode);
            curnode = curnode->children[0].first;
        }
        int win = rollout(curnode, root->cur_player);
        backPropagation(curnode, win);
    }
    std::cout << "iterations: " << iteration << "\n";
    std::cout << root->partial_wins << "/" << root->partial_games << " = " << (double)root->partial_wins / (root->partial_games + DIV_DELTA) << "\n";
}

Node* traversal(Node* root) {
    Node* curnode = root;
    while (!curnode->isLeaf) {
        Node* tmpnode = nullptr;
        double max_UCT = -1e306;
        for (auto child : curnode->children) {
            double tmp = ((double)child.first->partial_wins / (child.first->partial_games + DIV_DELTA)) + sqrt(2 * log(*child.first->total_games) / (child.first->partial_games + DIV_DELTA));
            if (tmp > max_UCT) {
                max_UCT = tmp;
                tmpnode = child.first;
            }
        }
        curnode = tmpnode;
    }
    return curnode;
}

void expansion(Node* node) {
    if (!node->isLeaf)
        return;
    node->isLeaf = false;

    if (node->children.empty())
#pragma omp parallel for schedule(dynamic) num_threads(8) collapse(2)
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                if (((node->board[i >> 2] >> ((((i & 3) << 3) + j) << 1)) & 3) == EMPTY) {
                    bool point_availible = false;
                    for (int d = 0; d < 8; d++) {
                        Point dir = Point(directions[d << 1], directions[(d << 1) + 1]);
                        Point p = Point(i, j) + dir;
                        if (!(p.x & (~7)) && !(p.y & (~7))) {
                            if (((node->board[p.x >> 2] >> ((((p.x & 3) << 3) + p.y) << 1)) & 3) != 3 - node->cur_player)
                                continue;
                            p = p + dir;
                            while (!(p.x & (~7)) && !(p.y & (~7)) && ((node->board[p.x >> 2] >> ((((p.x & 3) << 3) + p.y) << 1)) & 3) != EMPTY) {
                                if (((node->board[p.x >> 2] >> ((((p.x & 3) << 3) + p.y) << 1)) & 3) == node->cur_player) {
                                    point_availible = true;
                                    break;
                                }
                                p = p + dir;
                            }
                        }
                    }
                    if (point_availible) {
#pragma omp critical
                        node->children.emplace_back(nullptr, Point(i, j));
                    }
                }
            }
        }

    if (node->children.empty()) {
        Node* child_node = new Node(node);
        child_node->cur_player = 3 - child_node->cur_player;
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
                        if (((node->board[p.x >> 2] >> ((((p.x & 3) << 3) + p.y) << 1)) & 3) == node->cur_player) {
                            p = p - dir;
                            while (p != node->children[idx].second) {
                                child_node->disc_count[child_node->cur_player]++;
                                child_node->disc_count[3 - child_node->cur_player]--;
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
                child_node->disc_count[child_node->cur_player]++;
                child_node->board[p.x >> 2] &= ~((int64_t)3 << ((((p.x & 3) << 3) + p.y) << 1));
                child_node->board[p.x >> 2] |= ((int64_t)child_node->cur_player << ((((p.x & 3) << 3) + p.y) << 1));
                child_node->cur_player = 3 - child_node->cur_player;
                child_node->parent = node;
                node->children[idx].first = child_node;
            }
        }
    }
}

int rollout(Node* node, int player) {
    Node* curnode = node;
    while (!isTerminal(curnode)) {
        if (curnode->children.empty())
#pragma omp parallel for schedule(dynamic) collapse(2)
            for (int i = 0; i < SIZE; i++) {
                for (int j = 0; j < SIZE; j++) {
                    if (((curnode->board[i >> 2] >> ((((i & 3) << 3) + j) << 1)) & 3) == EMPTY) {
                        bool point_availible = false;
                        for (int d = 0; d < 8; d++) {
                            Point dir = Point(directions[d << 1], directions[(d << 1) + 1]);
                            Point p = Point(i, j) + dir;
                            if (!(p.x & (~7)) && !(p.y & (~7))) {
                                if (((curnode->board[p.x >> 2] >> ((((p.x & 3) << 3) + p.y) << 1)) & 3) != 3 - curnode->cur_player)
                                    continue;
                                p = p + dir;
                                while (!(p.x & (~7)) && !(p.y & (~7)) && ((curnode->board[p.x >> 2] >> ((((p.x & 3) << 3) + p.y) << 1)) & 3) != EMPTY) {
                                    if (((curnode->board[p.x >> 2] >> ((((p.x & 3) << 3) + p.y) << 1)) & 3) == curnode->cur_player) {
                                        point_availible = true;
                                        break;
                                    }
                                    p = p + dir;
                                }
                            }
                        }
                        if (point_availible) {
#pragma omp critical
                            curnode->children.emplace_back(nullptr, Point(i, j));
                        }
                    }
                }
            }

        if (curnode->children.empty()) {
            Node* child_node = new Node(curnode);
            child_node->cur_player = 3 - child_node->cur_player;
            child_node->parent = curnode;
            curnode->children.emplace_back(child_node, Point(-1, -1));
            curnode = curnode->children[0].first;
        } else {
            size_t idx = rand() % curnode->children.size();
            if (curnode->children[idx].first == nullptr) {
                Node* child_node = new Node(curnode);
                for (int d = 0; d < 8; d++) {
                    Point dir = Point(directions[d << 1], directions[(d << 1) + 1]);
                    Point p = curnode->children[idx].second + dir;

                    while (!(p.x & (~7)) && !(p.y & (~7)) && ((curnode->board[p.x >> 2] >> ((((p.x & 3) << 3) + p.y) << 1)) & 3) != EMPTY) {
                        if (((curnode->board[p.x >> 2] >> ((((p.x & 3) << 3) + p.y) << 1)) & 3) == curnode->cur_player) {
                            p = p - dir;
                            while (p != curnode->children[idx].second) {
                                child_node->disc_count[child_node->cur_player]++;
                                child_node->disc_count[3 - child_node->cur_player]--;
                                child_node->board[p.x >> 2] ^= (int64_t)3 << ((((p.x & 3) << 3) + p.y) << 1);
                                p = p - dir;
                            }
                            break;
                        }
                        p = p + dir;
                    }
                }
                Point p = curnode->children[idx].second;
                child_node->disc_count[0]--;
                child_node->disc_count[child_node->cur_player]++;
                child_node->board[p.x >> 2] &= ~((int64_t)3 << ((((p.x & 3) << 3) + p.y) << 1));
                child_node->board[p.x >> 2] |= ((int64_t)child_node->cur_player << ((((p.x & 3) << 3) + p.y) << 1));
                child_node->cur_player = 3 - child_node->cur_player;
                child_node->parent = curnode;
                curnode->children[idx].first = child_node;
            }
            curnode = curnode->children[idx].first;
        }
    }
    return (curnode->disc_count[player] - curnode->disc_count[3 - player] > 0 ? 1 : 0);
}

void backPropagation(Node* node, int win) {
    Node* curnode = node;
    while (curnode != nullptr) {
        curnode->partial_wins += win;
        curnode->partial_games++;
        curnode = curnode->parent;
    }
}