#include <bits/stdc++.h>

// #include "src/LOG.h"
#define MIN -0x7EFFFFFF
#define MAX 0x7EFFFFFF
const int SIZE = 8;
using State = std::array<std::array<int, SIZE>, SIZE>;

struct LOCALTIME {
    static std::chrono::_V2::system_clock::time_point start;
    static double get_duration() {
        return std::chrono::duration<double>(std::chrono::system_clock::now() - start).count();
    }
    static void initialize() {
        start = std::chrono::system_clock::now();
    }
};
std::chrono::_V2::system_clock::time_point LOCALTIME::start;
/**
 * @brief Stores: x, y.
 */
struct Point {
   public:
    // Operators
    bool operator==(const Point& rhs) const {
        return x == rhs.x && y == rhs.y;
    }
    bool operator!=(const Point& rhs) const {
        return !operator==(rhs);
    }
    bool operator<(const Point& rhs) const {
        return x < rhs.x || (x == rhs.x && y < rhs.y);
    }
    bool operator>(const Point& rhs) const {
        return operator!=(rhs) && !operator<(rhs);
    }
    Point operator+(const Point& rhs) const {
        return Point(x + rhs.x, y + rhs.y);
    }
    Point operator-(const Point& rhs) const {
        return Point(x - rhs.x, y - rhs.y);
    }
    Point& operator=(const Point& rhs) {
        x = rhs.x, y = rhs.y;
        return (*this);
    }
    int& operator[](const size_t n) {
        if (n == 0) return x;
        if (n == 1) return y;
    }
    friend std::ostream& operator<<(std::ostream&, const Point&);

    int x, y;
    Point() : Point(0, 0) {}
    Point(int x, int y) : x(x), y(y) {}
    Point(const Point& pt) : x(pt.x), y(pt.y) {}
};

/**
 * @brief Board class. Stores: board, curPlayer, value
 */
class Board {
   public:
    // Operators
    friend class Engine;
    friend std::ostream& operator<<(std::ostream&, const Board&);
    bool operator==(const Board& rhs) const {
        return this->board == rhs.board;
    }
    bool operator!=(const Board& rhs) const {
        return !operator==(rhs);
    }
    bool operator<(const Board& rhs) const {
        return 64 - this->get_cnt_discs(0) < 64 - rhs.get_cnt_discs(0);
    }
    bool operator>(const Board& rhs) const {
        return 64 - this->get_cnt_discs(0) > 64 - rhs.get_cnt_discs(0);
    }
    Board& operator=(const Board& rhs) {  // REMEMBER change when add new variables
        this->board = rhs.board;
        this->discsCnt = rhs.discsCnt;
        return *this;
    }
    const constexpr std::array<int, SIZE> operator[](int idx) const {
        return this->board[idx];
    }
    const std::array<Point, 8> directions{{Point(-1, -1), Point(-1, 0), Point(-1, 1),
                                           Point(0, -1), /*{0, 0}, */ Point(0, 1),
                                           Point(1, -1), Point(1, 0), Point(1, 1)}};
    enum SPOT_STATE {
        EMPTY = 0,
        BLACK = 1,
        WHITE = 2
    };

   private:
    State board;
    std::array<int, 3> discsCnt;

   public:
    Board() {
        discsCnt = {0, 0, 0};
    }
    Board(const Board& other) {
        Board();
        operator=(other);
    }
    ~Board() = default;
    void initialize() {
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                discsCnt[board[i][j]]++;
            }
        }
    }

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
        discsCnt[get_disc(p)]--;
        discsCnt[disc]++;
        board[p.x][p.y] = disc;
    }
    void flip_discs(Point center) {
        int player = get_disc(center);
        for (Point dir : directions) {
            Point p = center + dir;
            if (!is_disc_at(p, get_next_player(player)))
                continue;
            std::vector<Point> discs({p});
            p = p + dir;
            while (is_spot_on_board(p) && get_disc(p) != EMPTY) {
                if (is_disc_at(p, player)) {
                    for (Point s : discs) {
                        set_disc(s, player);
                    }
                    break;
                }
                discs.push_back(p);
                p = p + dir;
            }
        }
    }
    void set_move(Point p, int player) {
        set_disc(p, player);
        flip_discs(p);
    }
    bool is_disc_at(Point p, int disc) const {
        if (!is_spot_on_board(p))
            return false;
        if (get_disc(p) != disc)
            return false;
        return true;
    }
    bool is_spot_valid(Point center, int player) const {
        if (get_disc(center) != EMPTY)
            return false;
        for (Point dir : directions) {
            // Move along the direction while testing.
            Point p = center + dir;
            if (!is_disc_at(p, get_next_player(player)))
                continue;
            p = p + dir;
            while (is_spot_on_board(p) && get_disc(p) != EMPTY) {
                if (is_disc_at(p, player))
                    return true;
                p = p + dir;
            }
        }
        return false;
    }
    std::vector<Point> get_valid_spots(int player) const {
        std::vector<Point> valid_spots;
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                Point p = Point(i, j);
                if (board[i][j] != EMPTY)
                    continue;
                if (is_spot_valid(p, player))
                    valid_spots.push_back(p);
            }
        }
        return valid_spots;
    }
    bool is_terminal() const {
        if (this->discsCnt[0] == 0) return true;
        int toCheck = discsCnt[0];
        for (int i = 0; i < SIZE && toCheck; i++) {
            for (int j = 0; j < SIZE && toCheck; j++) {
                if (board[i][j] != EMPTY)
                    continue;
                toCheck--;
                Point curPt = Point(i, j);
                for (auto dir : directions) {
                    Point p = curPt + dir;
                    int firstPly = get_disc(p);
                    while (is_spot_on_board(p) && get_disc(p) != EMPTY) {
                        if (get_disc(p) != firstPly) return false;
                        p = p + dir;
                    }
                }
            }
        }
        return true;
    }
    int get_cnt_discs(int i) const {
        return discsCnt[i];
    }
};

std::ostream& operator<<(std::ostream& os, const Point& rhs) {
    os << "(" << rhs.x << "," << rhs.y << ")";
    return os;
}
std::ostream& operator<<(std::ostream& os, const Board& rhs) {
    os << "+---------------+\n";
    for (int i = 0; i < SIZE; i++) {
        os << "|";
        for (int j = 0; j < SIZE; j++) {
            switch (rhs[i][j]) {
                case 1:
                    os << "O";
                    break;
                case 2:
                    os << "X";
                    break;
                default:
                    os << " ";
                    break;
            }
            if (j != SIZE - 1) os << " ";
        }
        os << "|\n";
    }
    os << "+---------------+\n";
    return os;
}

/**
 * @brief Input output dealer
 * 
 */
struct Engine {
    static std::ifstream fin;
    static std::ofstream fout;
    static int curPlayer;
    static Board curBoard;
    static std::vector<Point> nxtSpots;

    static void read_board() {
        fin >> curPlayer;
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                fin >> curBoard.board[i][j];
            }
        }
    }
    static void read_spots() {
        int n_valid_spots;
        fin >> n_valid_spots;
        int x, y;
        for (int i = 0; i < n_valid_spots; i++) {
            fin >> x >> y;
            nxtSpots.push_back({x, y});
        }
    }
    static void write_spot(Point pt) {
        fout << pt.x << " " << pt.y << std::endl;
        fout.flush();
    }
};
std::ifstream Engine::fin;
std::ofstream Engine::fout;
int Engine::curPlayer;
Board Engine::curBoard;
std::vector<Point> Engine::nxtSpots;

/**
 * @brief Virtual Base Class Of the methods, Stores: curPlayer, curBoard, nxtSpots
 * 
 */
class AIMethod {
   protected:
    int curPlayer;
    Board curBoard;
    std::vector<Point> nxtSpots;

   public:
    AIMethod() {
        this->curPlayer = Engine::curPlayer;
        this->curBoard = Engine::curBoard;
        this->nxtSpots = Engine::nxtSpots;
    }
    virtual ~AIMethod() {}
    virtual void solve() = 0;

   private:
    enum TYPE_ID {
        ID_CORNER,
        ID_CENT_A,
        ID_CENT_B,
        ID_EDGE_A,
        ID_EDGE_B,
        ID_CORN_X,
        ID_CORN_C,
        ID_EDGE_0,
        ID_EDGE_1
    };
    const State BOARD_ID = {{
        {ID_CORNER, ID_CORN_C, ID_EDGE_B, ID_EDGE_A, ID_EDGE_A, ID_EDGE_B, ID_CORN_C, ID_CORNER},
        {ID_CORN_C, ID_CORN_X, ID_EDGE_1, ID_EDGE_0, ID_EDGE_0, ID_EDGE_1, ID_CORN_X, ID_CORN_C},
        {ID_EDGE_B, ID_EDGE_1, ID_CENT_B, ID_CENT_A, ID_CENT_A, ID_CENT_B, ID_EDGE_1, ID_EDGE_B},
        {ID_EDGE_A, ID_EDGE_0, ID_CENT_A, ID_CENT_A, ID_CENT_A, ID_CENT_A, ID_EDGE_0, ID_EDGE_A},
        {ID_EDGE_A, ID_EDGE_0, ID_CENT_A, ID_CENT_A, ID_CENT_A, ID_CENT_A, ID_EDGE_0, ID_EDGE_A},
        {ID_EDGE_B, ID_EDGE_1, ID_CENT_B, ID_CENT_A, ID_CENT_A, ID_CENT_B, ID_EDGE_1, ID_EDGE_B},
        {ID_CORN_C, ID_CORN_X, ID_EDGE_1, ID_EDGE_0, ID_EDGE_0, ID_EDGE_1, ID_CORN_X, ID_CORN_C},
        {ID_CORNER, ID_CORN_C, ID_EDGE_B, ID_EDGE_A, ID_EDGE_A, ID_EDGE_B, ID_CORN_C, ID_CORNER},
    }};
    const std::array<std::array<Point, 4>, 4> CORNERS = {{
        {{{0, 0}, {0, 1}, {1, 0}, {1, 1}}},
        {{{7, 0}, {7, 1}, {6, 0}, {6, 1}}},
        {{{7, 7}, {7, 6}, {6, 7}, {6, 6}}},
        {{{0, 7}, {0, 6}, {1, 7}, {1, 6}}},
    }};
    int get_id(Point p) const {
        return BOARD_ID[p.x][p.y];
    }

   protected:
    int sum(int a, int b) const {
        if (a < 0 && b < 0 && a + b > 0) return MIN;
        if (a > 0 && b > 0 && a + b < 0) return MAX;
        return a + b;
    }
    int get_stage(Board& board, int player) const {
        std::vector<int> row = {0, 1, 6, 7};
        int stage = 1;
        for (auto i : row) {
            for (int j = 0; j < SIZE && stage == 1; j++) {
                if (board[i][j] == player && (BOARD_ID[i][j] != ID_CENT_A || BOARD_ID[i][j] != ID_CENT_B || BOARD_ID[i][j] != ID_EDGE_1)) stage = 2;
            }
            if (stage != 1) break;
        }
        return stage;
    }
    int evaluate(Board& board, int player) const {
        int val = 0;
        int stage = get_stage(board, player);
        if (stage == 1) {
            val += get_mobility(board, player);
        } else {
            val += get_value(board, player);
        }
        return val;
    }
    int get_mobility(Board& board, int player) const {
        int cnt = board.get_valid_spots(player).size() - 3;
        return cnt < 0 ? cnt * 120 : cnt * 100;
    }
    int get_value(Board& board, int player) const {
        int val = 0;
        for (auto& CORNERARR : CORNERS) {
            int corn[4];
            for (int i = 0; i < 4; i++) corn[i] = board.get_disc(CORNERARR[i]);
            if (corn[0] == 0) {
                if (corn[1]) val += (corn[1] == player ? -1 : 1) * 700;
                if (corn[2]) val += (corn[2] == player ? -1 : 1) * 700;
                if (corn[3]) val += (corn[3] == player ? -1 : 1) * 1000;
            } else {
                val += (corn[0] == player ? 1 : -1) * 4000;
                if (corn[1]) val += (corn[1] == player ? 1 : -1) * 200;
                if (corn[2]) val += (corn[2] == player ? 1 : -1) * 200;
                // if (corn[3]) val += (corn[3] == player ? 1 : -1) * 100;
            }
        }
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                switch (BOARD_ID[i][j]) {
                    case ID_CORNER:
                        break;
                    case ID_CORN_X:
                        break;
                    case ID_CORN_C:
                        break;
                    case ID_CENT_A:
                        val += (board[i][j] == player ? 1 : -1) * 100;
                        break;
                    case ID_CENT_B:
                        val += (board[i][j] == player ? 1 : -1) * -100;
                        break;
                    case ID_EDGE_A:
                        val += (board[i][j] == player ? 1 : -1) * 100;
                        break;
                    case ID_EDGE_B:
                        val += (board[i][j] == player ? 1 : -1) * 500;
                        break;
                    case ID_EDGE_0:
                        val += (board[i][j] == player ? 1 : -1) * -100;
                        break;
                    case ID_EDGE_1:
                        val += (board[i][j] == player ? 1 : -1) * -200;
                        break;
                    default:
                        break;
                }
            }
        }
        return val;
    }
};
class AIAlphaBetaPruning : public AIMethod {
   public:
    AIAlphaBetaPruning() {
    }
    void solve() override {
        MAXDEPTH = 19;
        if (this->curBoard.get_cnt_discs(0) > MAXDEPTH)
            MAXDEPTH = 8;
        // LOG() << "Maxdepth: " << MAXDEPTH << "\n";
        this->getAlphaBetaVal(this->curBoard, 0, MIN, MAX, this->curPlayer);
    }

   private:
    int MAXDEPTH;
    int getAlphaBetaVal(Board curBoard, int depth, int alpha, int beta, int player) const {
        int retVal = 0;
        if (curBoard.is_terminal()) {
            int a = curBoard.get_cnt_discs(player);
            int b = curBoard.get_cnt_discs(3 - player);
            if (a > b)
                retVal = MAX;
            else if (a < b)
                retVal = MIN;
            else if (a == b)
                retVal = 0;
        } else if (depth >= MAXDEPTH) {
            retVal = evaluate(curBoard, player);
        } else {
            std::vector<Point> nxtSpots;
            Board nxtBoard;
            int maxVal, nxtVal;

            maxVal = MIN;
            nxtSpots = (depth == 0 ? this->nxtSpots : curBoard.get_valid_spots(player));

            if (nxtSpots.size() == 0) {
                maxVal = -this->getAlphaBetaVal(curBoard, depth, -beta, -alpha, 3 - player);
            } else {
                for (auto spot : nxtSpots) {
                    nxtBoard = curBoard;
                    nxtBoard.set_move(spot, player);
                    nxtVal = -this->getAlphaBetaVal(nxtBoard, depth + 1, -beta, -alpha, 3 - player);
                    if (nxtVal > maxVal) {
                        if (depth == 0) {
                            Engine::write_spot(spot);
                            // LOG() << "Value of " << player << "-" << 3 - player << " " << spot << " is " << nxtVal << "\n";
                        }
                        maxVal = nxtVal;
                    }
                    alpha = std::max(alpha, maxVal);
                    if (beta <= alpha) break;
                }
            }

            retVal = maxVal;
        }
        return retVal;
    }
};

int main(int, char** argv) {
    Engine::fin.open(argv[1]);                      // Input file stream
    Engine::fout.open(argv[2]);                     // Output file stream
    Engine::read_board();                           // Read the board from fin
    Engine::read_spots();                           // Read Valid inputs from fin
    Engine::write_spot(*Engine::nxtSpots.begin());  // Output once first to prevent running too long
    Engine::curBoard.initialize();

    // {
    //     LOCALTIME::initialize();
    //     LOG::initialize();
    //     LOG() << "╰(*°▽°*)╯(❁´◡`❁)(●'◡'●)\n"
    //           << "Current Player: " << (Engine::curPlayer == 1 ? "O" : "X") << "\n"
    //           << "Current Board:\n"
    //           << Engine::curBoard
    //           << "Valid moves: ";
    //     for (auto it = Engine::nxtSpots.begin(); it != Engine::nxtSpots.end(); it++) {
    //         auto nxtit = it;
    //         LOG() << (*it) << (++nxtit != Engine::nxtSpots.end() ? ", " : "\n");
    //     }
    // }

    // Start AI;
    AIMethod* aiMethod = new AIAlphaBetaPruning();
    aiMethod->solve();
    delete aiMethod;

    Engine::fin.close();
    Engine::fout.close();

    // {
    //     LOG() << "Duration: " << LOCALTIME::get_duration() << "s\n";
    //     LOG() << "=================\n";
    //     LOG::terminate();
    // }

    return 0;
}