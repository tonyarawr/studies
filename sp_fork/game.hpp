#include <string>
#include <iostream>

class Player
{
private:
    char symbol;
    std::string name;
public: 
    Player() : symbol(' '), name("") {};
    Player(char sym, std::string n) : symbol(sym), name(n) {}

    char getSymbol() const { return symbol; }
    std::string getName() const { return name; }
};

class Board {
private:
    char grid[3][3];
    int filledCells; 
    
public:

    Board() : filledCells(0) {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                grid[i][j] = ' ';
            }
        }
    }

    void drawBoard() const {
        for (int k = 0; k < 3; k++) { std::cout << "+---"; }
        std::cout << "+";
        std::cout << "\n";

        for (int i = 0; i < 3; i++) {
            std::cout << "| ";
            for (int j = 0; j < 3; j++) {
                std::cout << grid[i][j] << " | ";
            }
            std::cout << "\n";
            for (int k = 0; k < 3; k++) { std::cout << "+---"; }
            std::cout << "+";
            std::cout << "\n";
        }
    }

    bool isValidMove(int row, int col) const {
        return (row >= 0 && row < 3 && col >= 0 && col < 3 && grid[row][col] == ' ');
    }

    void makeMove(int row, int col, char symbol) {
        if (isValidMove(row, col)) {
            grid[row][col] = symbol;
            filledCells++;
        }
    }

    bool checkWin(char symbol) const {
        for (int i = 0; i < 3; i++) {
            if (grid[i][0] == symbol && grid[i][1] == symbol && grid[i][2] == symbol) {
                return true;
            }
        }
        
        for (int i = 0; i < 3; i++) {
            if (grid[0][i] == symbol && grid[1][i] == symbol && grid[2][i] == symbol) {
                return true;
            }
        }
        
        if (grid[0][0] == symbol && grid[1][1] == symbol && grid[2][2] == symbol) {
            return true;
        }
        if (grid[0][2] == symbol && grid[1][1] == symbol && grid[2][0] == symbol) {
            return true;
        }
        
        return false;
    }

    bool isFull() const {
        return filledCells == 9;
    }
};

class Game
{
private:
    Board board;
    Player players[2];
    int currPlayerIndex;
    int gameSocket;
    bool amHost;
public:

    Game (char sym1, std::string first, char sym2, std::string second, int gs, bool host) : gameSocket(gs), currPlayerIndex(0), amHost(host)
    {
        players[0] = Player(sym1, first);
        players[1] = Player(sym2, second);
    }

    Player& getCurrentPlayer() {
        return players[currPlayerIndex];
    }

    void switchTurn() {
        currPlayerIndex = (currPlayerIndex + 1) % 2;
    }
    
    int play()
    {
        int position;
        std::cout << "ИГРА НАЧАЛАСЬ\n";

        while(!board.isFull())
        {
            board.drawBoard();
            Player& currentPlayer = getCurrentPlayer();

            bool myTurn = (amHost && currPlayerIndex == 0) || (!amHost && currPlayerIndex == 1);

            if(myTurn)
            {
                while (true) {
                    std::cout << currentPlayer.getName() << " (" << currentPlayer.getSymbol() 
                         << "), введите номер ячейки (1-9): ";
                    std::cin >> position;

                    int row = (position - 1) / 3;
                    int col = (position - 1) % 3;
                
                    if (board.isValidMove(row, col)) {
                        board.makeMove(row,col,currentPlayer.getSymbol());
                        send(gameSocket, &position, sizeof(position), 0);
                        break;
                    } else {
                        std::cout << "Недопустимый ход. Попробуйте снова\n";
                    }
                }     
            }
            else
            {
                std::cout << "Ожидание хода соперника...\n";
                int recev;
                if (recv(gameSocket, &recev, sizeof(recev), 0) <= 0) {
                    std::cout << "Связь оборвалась.\n";
                    return -1;
                }
                int row = (recev - 1) / 3;
                int col = (recev - 1) % 3;
                board.makeMove(row, col, currentPlayer.getSymbol());
            }

        if (board.checkWin(currentPlayer.getSymbol())) {
            board.drawBoard();
            std::cout << currentPlayer.getName() << " победил!\n";
            return currPlayerIndex;
        }
        switchTurn();
    }

        board.drawBoard();
        std::cout << "Победила дружба!\n";
        return 2;
    }

};
