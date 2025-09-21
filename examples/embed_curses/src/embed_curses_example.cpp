#include <cstdio>
#include <string>
#include <vector>
#include <pico/stdlib.h>
#include <embed_curses/embed_curses.hpp>

using namespace jsi::ecurses;

class DemoFont : public IFont
{
public:
    uint8_t        glyph_width() const override { return 6; }
    uint8_t        glyph_height() const override { return 8; }
    const uint8_t* glyph_bitmap(uint8_t ch) const override
    {
        static uint8_t box[8]   = {0x3E, 0x22, 0x22, 0x3E, 0x22, 0x22, 0x22, 0x3E};
        static uint8_t blank[8] = {0, 0, 0, 0, 0, 0, 0, 0};
        return (ch == ' ') ? blank : box;
    }
};

class AnsiTerminalDisplay : public ICursesDisplay
{
public:
    AnsiTerminalDisplay(int w, int h, int gw, int gh) : _wpx(w), _hpx(h), _gw(gw), _gh(gh), _first(true)
    {
        _cols = _wpx / _gw;
        _rows = _hpx / _gh;
        _grid.assign(_rows * _cols, Cell{' ', {255, 255, 255}, {0, 0, 0}, false, false});
    }

    int  width_px() const override { return _wpx; }
    int  height_px() const override { return _hpx; }
    void fill_rect(int, int, int, int, Color) override {}

    void draw_glyph(int x, int y, uint8_t ch, const uint8_t*, int gw, int gh, Color fg, Color bg, bool bold, bool rev)
        override
    {
        int col = x / gw, row = y / gh;
        if ((unsigned) col < (unsigned) _cols && (unsigned) row < (unsigned) _rows)
            _grid[row * _cols + col] = Cell{(ch >= 32 && ch < 127) ? (char) ch : ' ', fg, bg, bold, rev};
    }
    void invert_rect(int, int, int, int) override {}
    void present() override
    {
        if (_first)
        {
            std::printf("\x1b[2J\x1b[H");
            _first = false;
        }
        else
        {
            std::printf("\x1b[H");
        }
        for (int r = 0; r < _rows; ++r)
        {
            for (int c = 0; c < _cols; ++c)
                std::putchar(_grid[r * _cols + c].ch);
            std::putchar('\n');
        }
        std::printf("\x1b[H");
        std::fflush(stdout);
    }

private:
    struct Cell
    {
        char  ch;
        Color fg, bg;
        bool  bold, rev;
    };
    int               _wpx, _hpx, _cols, _rows;
    int               _gw, _gh;
    bool              _first;
    std::vector<Cell> _grid;
};

class StdioInput : public ICursesInput
{
public:
    int poll_key() override
    {
        int ch = getchar_timeout_us(0);
        return (ch == PICO_ERROR_TIMEOUT) ? KEY_NONE : ch;
    }
};

// --- DOS emulator state ---
static void execute_command(const std::string& cmd)
{
    if (cmd == "help")
    {
        printw("Commands: HELP VER CLS\r\n");
    }
    else if (cmd == "ver")
    {
        printw("ecurses DOS [Version 0.1]\r\n");
    }
    else if (cmd == "cls")
    {
        clear();
        refresh();
    }
    else if (!cmd.empty())
    {
        printw("Bad command or file name\r\n");
    }
}

static void prompt()
{
    attrset(COLOR_PAIR(1));
    printw("PICO:\\>");
}

int main()
{
    stdio_init_all();
    while (!stdio_usb_connected())
        sleep_ms(100);

    DemoFont            font;
    AnsiTerminalDisplay disp(240, 136, font.glyph_width(), font.glyph_height());
    StdioInput          input;
    Curses<>            screen(disp, input, font);
    set_active(screen);

    initscr();
    init_pair(1, Color{255, 255, 255}, Color{0, 0, 0});
    attrset(COLOR_PAIR(1));
    clear();
    prompt();
    refresh();

    std::string line;
    timeout(0);

    while (true)
    {
        int k = getch();
        if (k == KEY_NONE)
        {
            sleep_ms(16);
            continue;
        }
        if (k == '\r' || k == '\n')
        {
            printw("\r\n");
            execute_command(line);
            line.clear();
            prompt();
            refresh();
        }
        else if (k == 127 || k == 8)
        {
            if (!line.empty())
            {
                line.pop_back();
                int y, x;
                getyx(y, x);
                mvaddch(y, x - 1, ' ');
                move(y, x - 1);
                refresh();
            }
        }
        else if (k == 'q')
        {
            break;
        }
        else if (k >= 32 && k < 127)
        {
            line.push_back((char) k);
            addch((char) k);
            refresh();
        }
    }

    endwin();
    return 0;
}
