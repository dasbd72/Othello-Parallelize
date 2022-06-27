#include <array>
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

struct Point {
    int x, y;
    Point() : Point(0, 0) {}
    Point(float x, float y) : x(x), y(y) {}
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

class OthelloBoard {
   public:
    enum SPOT_STATE {
        EMPTY = 0,
        BLACK = 1,
        WHITE = 2
    };
    static const int SIZE = 8;
    const std::array<Point, 8> directions{{Point(-1, -1), Point(-1, 0), Point(-1, 1),
                                           Point(0, -1), /*{0, 0}, */ Point(0, 1),
                                           Point(1, -1), Point(1, 0), Point(1, 1)}};
    std::array<std::array<int, SIZE>, SIZE> board;
    std::vector<Point> next_valid_spots;
    std::array<int, 3> disc_count;
    int cur_player;
    bool done;
    int winner;

   private:
    int get_next_player(int player) const {
        return 3 - player;
    }
    bool is_spot_on_board(Point p) const {
        return 0 <= p.x && p.x < SIZE && 0 <= p.y && p.y < SIZE;
    }
    int get_disc(Point p) const {
        return board[p.x][p.y];
    }
    void set_disc(Point p, int disc) {
        board[p.x][p.y] = disc;
    }
    bool is_disc_at(Point p, int disc) const {
        if (!is_spot_on_board(p))
            return false;
        if (get_disc(p) != disc)
            return false;
        return true;
    }
    bool is_spot_valid(Point center) const {
        if (get_disc(center) != EMPTY)
            return false;
        for (Point dir : directions) {
            // Move along the direction while testing.
            Point p = center + dir;
            if (!is_disc_at(p, get_next_player(cur_player)))
                continue;
            p = p + dir;
            while (is_spot_on_board(p) && get_disc(p) != EMPTY) {
                if (is_disc_at(p, cur_player))
                    return true;
                p = p + dir;
            }
        }
        return false;
    }
    void flip_discs(Point center) {
        for (Point dir : directions) {
            // Move along the direction while testing.
            Point p = center + dir;
            if (!is_disc_at(p, get_next_player(cur_player)))
                continue;
            std::vector<Point> discs({p});
            p = p + dir;
            while (is_spot_on_board(p) && get_disc(p) != EMPTY) {
                if (is_disc_at(p, cur_player)) {
                    for (Point s : discs) {
                        set_disc(s, cur_player);
                    }
                    disc_count[cur_player] += discs.size();
                    disc_count[get_next_player(cur_player)] -= discs.size();
                    break;
                }
                discs.push_back(p);
                p = p + dir;
            }
        }
    }

   public:
    OthelloBoard() {
        reset();
    }
    void reset() {
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                board[i][j] = EMPTY;
            }
        }
        board[3][4] = board[4][3] = BLACK;
        board[3][3] = board[4][4] = WHITE;
        cur_player = BLACK;
        disc_count[EMPTY] = 8 * 8 - 4;
        disc_count[BLACK] = 2;
        disc_count[WHITE] = 2;
        next_valid_spots = get_valid_spots();
        done = false;
        winner = -1;
    }
    std::vector<Point> get_valid_spots() const {
        std::vector<Point> valid_spots;
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                Point p = Point(i, j);
                if (board[i][j] != EMPTY)
                    continue;
                if (is_spot_valid(p))
                    valid_spots.push_back(p);
            }
        }
        return valid_spots;
    }
    bool put_disc(Point p) {
        if (!is_spot_valid(p)) {
            winner = get_next_player(cur_player);
            done = true;
            return false;
        }
        set_disc(p, cur_player);
        disc_count[cur_player]++;
        disc_count[EMPTY]--;
        flip_discs(p);
        // Give control to the other player.
        cur_player = get_next_player(cur_player);
        next_valid_spots = get_valid_spots();
        // Check Win
        if (next_valid_spots.size() == 0) {
            cur_player = get_next_player(cur_player);
            next_valid_spots = get_valid_spots();
            if (next_valid_spots.size() == 0) {
                // Game ends
                done = true;
                int white_discs = disc_count[WHITE];
                int black_discs = disc_count[BLACK];
                if (white_discs == black_discs)
                    winner = EMPTY;
                else if (black_discs > white_discs)
                    winner = BLACK;
                else
                    winner = WHITE;
            }
        }
        return true;
    }
    std::string encode_player(int state) {
        if (state == BLACK)
            return "O";
        if (state == WHITE)
            return "X";
        return "Draw";
    }
    std::string encode_spot(int x, int y) {
        if (is_spot_valid(Point(x, y)))
            return ".";
        if (board[x][y] == BLACK)
            return "O";
        if (board[x][y] == WHITE)
            return "X";
        return " ";
    }
    std::string encode_state() {
        int i, j;
        std::stringstream ss;
        ss << cur_player << "\n";
        for (i = 0; i < SIZE; i++) {
            for (j = 0; j < SIZE - 1; j++) {
                ss << board[i][j] << " ";
            }
            ss << board[i][j] << "\n";
        }
        return ss.str();
    }
};

std::string file_state = "tmp/state";
std::string file_action = "tmp/action";
const int num_players = 4;
const std::string file_players[] = {"./player", "./player_leaf_parallel", "./player_root_parallel", "./player_tree_parallel"};
// Timeout is set to 10 when TA test your code.
// const int timeout = 1000;

void launch_executable(int fileid, std::string preargs, std::string postargs) {
    std::string command = preargs + file_players[fileid] + " " + file_state + " " + file_action + postargs;
    std::cout << std::string("> ") + command + std::string("\n");
    system(command.c_str());
}

int main(int argc, char** argv) {
    assert(argc == 5);

    srand(time(NULL));
    int randomId = rand();
    file_state += "_" + std::to_string(randomId);
    file_action += "_" + std::to_string(randomId);

    int player_fileid[3] = {};
    player_fileid[1] = atoi(argv[1]);
    player_fileid[2] = atoi(argv[2]);

    int statistic[num_players] = {};
    int draw_cnt = 0;

    for (int gameid = 0; gameid < 20; gameid++) {
        std::cout << "Gameid: " << gameid << std::endl;
        std::cout << "Player Black FileId: " << player_fileid[OthelloBoard::BLACK] << std::endl;
        std::cout << "Player White FileId: " << player_fileid[OthelloBoard::WHITE] << std::endl;
        OthelloBoard game;
        while (!game.done) {
            std::string data = game.encode_state();
            std::ofstream fout(file_state);
            fout << data;
            fout.close();

            launch_executable(player_fileid[game.cur_player], std::string("srun -c") + std::string(argv[3]) + std::string(" "), std::string(" ") + std::string(argv[4]));

            std::ifstream fin(file_action);
            Point p(-1, -1);
            fin >> p.x >> p.y;
            fin.close();
            if (remove(file_action.c_str()) != 0) {
                std::cerr << "Error removing file: " << file_action << "\n";
                exit(1);
            }
            if (!game.put_disc(p))  // invalid action
                break;
        }
        if (game.winner) {
            std::cout << player_fileid[game.winner] << "\n";
            statistic[player_fileid[game.winner]]++;
        } else {
            draw_cnt++;
        }
        // Reset state file
        if (remove(file_state.c_str()) != 0) {
            std::cerr << "Error removing file: " << file_state << "\n";
            exit(1);
        }
        std::swap(player_fileid[1], player_fileid[2]);
    }
    std::cout << atoi(argv[1]) << ": " << statistic[atoi(argv[1])] << "\n";
    std::cout << atoi(argv[2]) << ": " << statistic[atoi(argv[2])] << "\n";
    std::cout << "Draw: " << draw_cnt << "\n";
    return 0;
}
