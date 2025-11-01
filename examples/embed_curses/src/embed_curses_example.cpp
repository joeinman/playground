#include <cstdio>
#include <string>
#include <vector>
#include <algorithm>
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
            std::printf("\x1b[2J");
            _first = false;
        }
        for (int r = 0; r < _rows; ++r)
        {
            std::printf("\x1b[%d;1H", r + 1);
            for (int c = 0; c < _cols; ++c)
                std::putchar(_grid[r * _cols + c].ch);
            std::printf("\x1b[K");
        }
        std::printf("\x1b[%d;1H\x1b[J", _rows + 1);
        std::fflush(stdout);
    }

    void resize_chars(int cols, int rows)
    {
        _cols = std::max(1, cols);
        _rows = std::max(1, rows);
        _wpx  = _cols * _gw;
        _hpx  = _rows * _gh;
        _grid.assign(_rows * _cols, Cell{' ', {255, 255, 255}, {0, 0, 0}, false, false});
        _first = true;
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

static void draw_status(WINDOW* status, uint32_t uptime_s, size_t command_count, bool recalling)
{
    if (!status)
        return;
    int h = 0, w = 0;
    getmaxyx(status, h, w);
    const int row = (h > 1) ? 1 : 0;
    const int col = (w > 1) ? 1 : 0;
    wattrset(status, COLOR_PAIR(3) | A_BOLD);
    wmove(status, row, col);
    wclrtoeol(status);
    wprintw(status,
            "EmbedCurses demo  |  uptime %lus  |  commands %zu  |  history %s",
            uptime_s,
            command_count,
            recalling ? "RECALL" : "LIVE");
}

static void render_console(WINDOW* win, const std::vector<std::string>& lines)
{
    if (!win)
        return;
    int h = 0, w = 0;
    getmaxyx(win, h, w);
    if (h <= 0 || w <= 0)
        return;
    const int start = (lines.size() > static_cast<size_t>(h)) ? (lines.size() - h) : 0;
    for (int row = 0; row < h; ++row)
    {
        const int idx = start + row;
        wmove(win, row, 0);
        wclrtoeol(win);
        if (idx < static_cast<int>(lines.size()))
        {
            const std::string& line = lines[idx];
            if (static_cast<int>(line.size()) > w)
            {
                std::string truncated(line.begin(), line.begin() + w);
                waddstr(win, truncated.c_str());
            }
            else
            {
                waddstr(win, line.c_str());
            }
        }
    }
}

static void render_history(WINDOW* win, const std::vector<std::string>& history)
{
    if (!win)
        return;

    int h = 0, w = 0;
    getmaxyx(win, h, w);
    if (h <= 0 || w <= 0)
        return;

    const int base_col   = (w > 1) ? 1 : 0;
    const int header_row = (h > 1) ? 1 : 0;

    wattrset(win, COLOR_PAIR(1) | A_BOLD);
    wmove(win, header_row, base_col);
    wclrtoeol(win);
    waddstr(win, "Recent commands:");

    int content_row = (header_row + 1 < h) ? header_row + 1 : header_row;
    int list_row    = content_row;
    int inner_width = w - (base_col ? 2 : 0);
    if (content_row < h && inner_width > 0)
    {
        wattrset(win, COLOR_PAIR(1));
        wmove(win, content_row, base_col);
        whline(win, ACS_HLINE, inner_width);
        list_row = content_row + 1;
    }

    const int rows_available = (list_row < h) ? (h - list_row) : 0;
    if (rows_available <= 0)
        return;

    wattrset(win, COLOR_PAIR(1));
    const int start =
        static_cast<int>((history.size() > static_cast<size_t>(rows_available)) ? history.size() - rows_available : 0);
    for (int row = 0; row < rows_available; ++row)
    {
        const int idx = start + row;
        wmove(win, list_row + row, base_col);
        wclrtoeol(win);
        if (idx < static_cast<int>(history.size()))
            waddstr(win, history[idx].c_str());
    }
}

static void render_input(WINDOW* input, const std::string& line)
{
    if (!input)
        return;
    int h = 0, w = 0;
    getmaxyx(input, h, w);
    const int row = (h > 1) ? 1 : 0;
    const int col = (w > 1) ? 1 : 0;
    wattrset(input, COLOR_PAIR(1) | A_BOLD);
    wmove(input, row, col);
    wclrtoeol(input);
    waddstr(input, "PICO:>");
    wattrset(input, COLOR_PAIR(1));
    if (!line.empty())
        waddstr(input, line.c_str());
    int underline_row = row + 1;
    int line_len      = w - (col ? 2 : 0);
    if (underline_row < h && line_len > 0)
    {
        wmove(input, underline_row, col);
        whline(input, ACS_HLINE, line_len);
    }
}

static void append_console_prompt(WINDOW* console, std::vector<std::string>& buffer, const std::string& line)
{
    if (!console)
        return;
    std::string formatted = "PICO:> " + line;
    buffer.push_back(formatted);
    wattrset(console, COLOR_PAIR(1) | A_BOLD);
    wprintw(console, "%s\r\n", formatted.c_str());
}

static void clear_console(WINDOW* frame, WINDOW* console, std::vector<std::string>& buffer)
{
    if (!console)
        return;
    wattrset(console, COLOR_PAIR(1));
    wclear(console);
    buffer.clear();
    if (frame && frame != console)
    {
        wattrset(frame, COLOR_PAIR(1));
        box(frame);
    }
}

static void run_command(WINDOW* console_frame,
                        WINDOW* console,
                        std::vector<std::string>& buffer,
                        const std::string&        cmd)
{
    if (!console || cmd.empty())
        return;

    if (cmd == "help")
    {
        wattrset(console, COLOR_PAIR(2));
        buffer.emplace_back("Commands: HELP VER CLS");
        wprintw(console, "Commands: HELP VER CLS\r\n");
    }
    else if (cmd == "ver")
    {
        wattrset(console, COLOR_PAIR(2));
        buffer.emplace_back("ecurses DOS [Version 0.2]");
        wprintw(console, "ecurses DOS [Version 0.2]\r\n");
    }
    else if (cmd == "cls")
    {
        clear_console(console_frame, console, buffer);
        wattrset(console, COLOR_PAIR(2));
        buffer.emplace_back("Console cleared.");
        wprintw(console, "Console cleared.\r\n");
    }
    else
    {
        wattrset(console, COLOR_PAIR(1));
        buffer.emplace_back("Unknown command: " + cmd);
        wprintw(console, "Unknown command: %s\r\n", cmd.c_str());
    }
}

int main()
{
    stdio_init_all();
    while (!stdio_usb_connected())
        sleep_ms(100);

    constexpr int SCREEN_MAX_COLS = 240;
    constexpr int SCREEN_MAX_ROWS = 120;

    DemoFont            font;
    AnsiTerminalDisplay disp(SCREEN_MAX_COLS * font.glyph_width(),
                             SCREEN_MAX_ROWS * font.glyph_height(),
                             font.glyph_width(),
                             font.glyph_height());
    StdioInput          input;
    Curses<>            screen(disp, input, font);
    set_active(screen);

    initscr();
    timeout(0);

    init_pair(1, Color{230, 230, 230}, Color{0, 0, 0});
    init_pair(2, Color{0, 255, 180}, Color{0, 0, 0});
    init_pair(3, Color{255, 255, 0}, Color{0, 0, 40});

    int cols = COLS();
    int rows = LINES();

    int status_height = std::min(3, rows);
    int input_height  = std::min(3, std::max(rows - status_height, 1));
    int middle_start  = status_height;
    int middle_height = std::max(rows - status_height - input_height, 1);

    int console_cols = std::max(cols / 2, 1);
    int history_cols = std::max(cols - console_cols, 1);
    if (console_cols + history_cols > cols)
        history_cols = std::max(cols - console_cols, 1);

    WINDOW* status_frame  = newwin(status_height, cols, 0, 0);
    WINDOW* console_frame = newwin(middle_height, console_cols, middle_start, 0);
    WINDOW* history_frame = newwin(middle_height, history_cols, middle_start, console_cols);
    WINDOW* input_frame   = newwin(input_height, cols, middle_start + middle_height, 0);

    if (!status_frame || !console_frame || !history_frame || !input_frame)
        return 1;

    auto apply_frame_border = [](WINDOW* frame, int height, int width, uint16_t attr_pair) {
        if (!frame)
            return;
        wattrset(frame, attr_pair);
        if (height > 1 && width > 1)
            box(frame);
    };
    WINDOW* status_win  = nullptr;
    WINDOW* console_win = nullptr;
    WINDOW* history_win = nullptr;
    WINDOW* input_win   = nullptr;

    bool history_dirty = true;
    bool input_dirty   = true;
    bool status_dirty  = true;
    bool console_dirty = true;

    constexpr int MIN_PANEL_COLS  = 10;
    constexpr int MIN_SCREEN_COLS = MIN_PANEL_COLS * 2;
    constexpr int MIN_SCREEN_ROWS = 5;
    const int     KEYMOD_MASK    = KEYMOD_SHIFT | KEYMOD_ALT | KEYMOD_CTRL;

    auto reconfigure_inner = [&](WINDOW*& inner, WINDOW* frame, int height, int width, uint16_t attr_pair) {
        if (!frame)
        {
            inner = nullptr;
            return;
        }
        if (inner && inner != frame)
            delwin(inner);
        if (height > 2 && width > 2)
            inner = derwin(frame, height - 2, width - 2, 1, 1);
        else
            inner = frame;
        if (inner)
        {
            wattrset(inner, attr_pair);
            werase(inner);
        }
    };

    auto apply_layout = [&](int desired_console_cols) {
        cols = COLS();
        rows = LINES();

        status_height = std::clamp(status_height, 1, std::max(1, rows - 2));
        input_height  = std::clamp(input_height, 1, std::max(1, rows - status_height - 1));
        middle_start  = status_height;
        middle_height = std::max(rows - status_height - input_height, 1);

        console_cols = std::clamp(desired_console_cols, MIN_PANEL_COLS, std::max(MIN_PANEL_COLS, cols - MIN_PANEL_COLS));
        history_cols = std::max(cols - console_cols, MIN_PANEL_COLS);
        if (console_cols + history_cols > cols)
        {
            history_cols = std::max(MIN_PANEL_COLS, cols / 2);
            console_cols = cols - history_cols;
        }

        screen.clear();

        wresize(status_frame, status_height, cols);
        mvwin(status_frame, 0, 0);
        wresize(console_frame, middle_height, console_cols);
        mvwin(console_frame, middle_start, 0);
        wresize(history_frame, middle_height, history_cols);
        mvwin(history_frame, middle_start, console_cols);
        wresize(input_frame, input_height, cols);
        mvwin(input_frame, middle_start + middle_height, 0);

        apply_frame_border(status_frame, status_height, cols, COLOR_PAIR(3));
        apply_frame_border(console_frame, middle_height, console_cols, COLOR_PAIR(1));
        apply_frame_border(history_frame, middle_height, history_cols, COLOR_PAIR(1));
        apply_frame_border(input_frame, input_height, cols, COLOR_PAIR(1));

        reconfigure_inner(status_win, status_frame, status_height, cols, COLOR_PAIR(3));
        reconfigure_inner(console_win, console_frame, middle_height, console_cols, COLOR_PAIR(2));
        reconfigure_inner(history_win, history_frame, middle_height, history_cols, COLOR_PAIR(1));
        reconfigure_inner(input_win, input_frame, input_height, cols, COLOR_PAIR(1));

        if (input_win)
            keypad(input_win, true);

        touchwin(status_frame);
        touchwin(console_frame);
        touchwin(history_frame);
        touchwin(input_frame);

        history_dirty = true;
        input_dirty   = true;
        status_dirty  = true;
        console_dirty = true;
    };

    auto apply_screen_resize = [&](int desired_rows, int desired_cols) {
        int target_rows = std::min(std::max(desired_rows, std::max(status_height + input_height + 1, MIN_SCREEN_ROWS)),
                                   SCREEN_MAX_ROWS);
        int target_cols = std::min(std::max(desired_cols, MIN_SCREEN_COLS), SCREEN_MAX_COLS);

        screen.resizeterm(target_rows, target_cols);
        disp.resize_chars(target_cols, target_rows);

        cols = COLS();
        rows = LINES();

        status_height = std::clamp(status_height, 1, std::max(1, rows - 2));
        input_height  = std::clamp(input_height, 1, std::max(1, rows - status_height - 1));
        middle_start  = status_height;
        middle_height = std::max(rows - status_height - input_height, 1);

        console_cols = std::clamp(console_cols, MIN_PANEL_COLS, std::max(MIN_PANEL_COLS, cols - MIN_PANEL_COLS));
        history_cols = cols - console_cols;

        apply_layout(console_cols);
    };

    apply_screen_resize(rows, cols);

    std::vector<std::string> history = {"Type 'help' to list commands",
                                        "Type 'ver' for a fake version",
                                        "Type 'cls' to clear the console",
                                        "Ctrl+Left/Right resize console/history",
                                        "Press 'q' to exit the demo"};
    std::vector<std::string> console_lines;
    console_lines.emplace_back("EmbedCurses console ready. Type HELP for commands.");
    console_dirty = true;

    std::string              line;
    size_t                   command_count = 0;
    std::vector<std::string> command_log;
    int                      recall_index = -1;
    bool                     quitting      = false;

    absolute_time_t next_status = make_timeout_time_ms(0);

    while (!quitting)
    {
        int  ch           = wgetch(input_win);
        bool need_present = false;

        if (ch != KEY_NONE)
        {
            if (ch == KEY_RESIZE)
            {
                int new_rows = rows;
                int new_cols = cols;
                if (screen.consume_resize(new_rows, new_cols))
                {
                    apply_screen_resize(new_rows, new_cols);
                    need_present = true;
                }
                continue;
            }

            int modifiers = ch & KEYMOD_MASK;
            int key       = ch & ~KEYMOD_MASK;

            if (modifiers & KEYMOD_CTRL)
            {
                if (key == KEY_LEFT && console_cols > MIN_PANEL_COLS)
                {
                    apply_layout(console_cols - 2);
                    need_present = true;
                    continue;
                }
                if (key == KEY_RIGHT && history_cols > MIN_PANEL_COLS)
                {
                    apply_layout(console_cols + 2);
                    need_present = true;
                    continue;
                }
                if (key == KEY_UP)
                {
                    apply_screen_resize(rows + 1, cols);
                    need_present = true;
                    continue;
                }
                if (key == KEY_DOWN)
                {
                    apply_screen_resize(rows - 1, cols);
                    need_present = true;
                    continue;
                }
            }

            if (key == '\r' || key == '\n' || key == KEY_ENTER)
            {
                append_console_prompt(console_win, console_lines, line);
                console_dirty = true;

                if (!line.empty())
                {
                    history.emplace_back("> " + line);
                    if (history.size() > 32)
                        history.erase(history.begin(), history.end() - 32);
                    history_dirty = true;
                    ++command_count;
                    command_log.push_back(line);
                }

                run_command(console_frame, console_win, console_lines, line);
                console_dirty = true;

                line.clear();
                input_dirty  = true;
                status_dirty = true;
                recall_index = -1;
            }
            else if (key == 127 || key == 8 || key == KEY_BACKSPACE)
            {
                if (!line.empty())
                {
                    line.pop_back();
                    input_dirty  = true;
                    status_dirty = true;
                }
            }
            else if (key == KEY_UP)
            {
                if (!command_log.empty())
                {
                    if (recall_index < 0)
                        recall_index = static_cast<int>(command_log.size()) - 1;
                    else if (recall_index > 0)
                        --recall_index;
                    line         = command_log[recall_index];
                    input_dirty  = true;
                    status_dirty = true;
                }
            }
            else if (key == KEY_DOWN)
            {
                if (!command_log.empty() && recall_index >= 0)
                {
                    if (recall_index < static_cast<int>(command_log.size()) - 1)
                    {
                        ++recall_index;
                        line = command_log[recall_index];
                    }
                    else
                    {
                        recall_index = -1;
                        line.clear();
                    }
                    input_dirty  = true;
                    status_dirty = true;
                }
            }
            else if (key == 'q' || key == 'Q')
            {
                history.emplace_back("> quit");
                history_dirty = true;
                console_lines.emplace_back("Exiting demo...");
                wattrset(console_win, COLOR_PAIR(1));
                wprintw(console_win, "Exiting demo...\r\n");
                console_dirty = true;
                quitting      = true;
            }
            else if (key >= 32 && key < 127)
            {
                line.push_back(static_cast<char>(key));
                input_dirty  = true;
                status_dirty = true;
                recall_index = -1;
            }
        }

        absolute_time_t now = get_absolute_time();
        if (status_dirty || absolute_time_diff_us(now, next_status) <= 0)
        {
            draw_status(status_win, to_ms_since_boot(now) / 1000, command_count, recall_index >= 0);
            status_dirty = false;
            next_status  = make_timeout_time_ms(250);
            need_present = true;
        }

        if (history_dirty)
        {
            render_history(history_win, history);
            history_dirty = false;
            need_present  = true;
        }

        if (input_dirty)
        {
            render_input(input_win, line);
            input_dirty  = false;
            need_present = true;
        }

        if (console_dirty)
        {
            render_console(console_win, console_lines);
            console_dirty = false;
            need_present  = true;
        }

        if (need_present)
        {
            wrefresh(input_win);
        }
        else
        {
            sleep_ms(16);
        }
    }

    wrefresh(input_win);

    if (input_win && input_win != input_frame)
        delwin(input_win);
    if (history_win && history_win != history_frame)
        delwin(history_win);
    if (console_win && console_win != console_frame)
        delwin(console_win);
    if (status_win && status_win != status_frame)
        delwin(status_win);

    delwin(input_frame);
    delwin(history_frame);
    delwin(console_frame);
    delwin(status_frame);

    endwin();
    return 0;
}
