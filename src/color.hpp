// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

/* This headers contain defines for UNIX terminal color to be used on server logs and client messages. */

/* Use this if to enable/disable colors */
# define USE_COLORS 1

/* Color definitions */
# if USE_COLORS

    # define COLOR_DEFAULT std::string("\033[0m")

    # define COLOR_RED std::string("\033[0;31m")
    # define COLOR_BOLD_RED std::string("\033[1;31m")

    # define COLOR_GREEN std::string("\033[0;32m")
    # define COLOR_BOLD_GREEN std::string("\033[1;32m")

    # define COLOR_YELLOW std::string("\033[0;33m")
    # define COLOR_BOLD_YELLOW std::string("\033[1;33m")

    # define COLOR_BLUE std::string("\033[0;34m")
    # define COLOR_BOLD_BLUE std::string("\033[1;34m")

    # define COLOR_MAGENTA std::string("\033[0;35m")
    # define COLOR_BOLD_MAGENTA std::string("\033[1;35m")

    # define COLOR_CYAN std::string("\033[0;36m")
    # define COLOR_BOLD_CYAN std::string("\033[1;36m")

# else // If colors are not to be used define them to empty strings.

    # define COLOR_DEFAULT std::string("");
    # define COLOR_RED std::string("");
    # define COLOR_BOLD_RED std::string("");
    # define COLOR_GREEN std::string("");
    # define COLOR_BOLD_GREEN std::string("");
    # define COLOR_YELLOW std::string("");
    # define COLOR_BOLD_YELLOW std::string("");
    # define COLOR_BLUE std::string("");
    # define COLOR_BOLD_BLUE std::string("");
    # define COLOR_MAGENTA std::string("");
    # define COLOR_BOLD_MAGENTA std::string("");
    # define COLOR_CYAN std::string("");
    # define COLOR_BOLD_CYAN std::string("");

# endif