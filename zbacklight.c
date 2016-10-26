#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#define CURRENT_BRIGHTNESS "/sys/class/backlight/intel_backlight/actual_brightness"
#define MAX_BRIGHTNESS "/sys/class/backlight/intel_backlight/max_brightness"
#define TARGET_BRIGHTNESS "/sys/class/backlight/intel_backlight/brightness"


typedef enum
{
    Get,
    Set,
    Inc,
    Dec
} op_type;

struct options
{
    op_type operation;
    float   value;
};

struct br_info
{
    int current;
    int max;
};


struct options parse_options(int ac, char * const * const av)
{
    struct options options;

    if (ac < 2)
    {
        fprintf(stderr, "Missing argument.\n");
        exit(1);
    }

    if (strcmp(av[1], "-get") == 0)
        options.operation = Get;
    else
    {
        if (ac < 3)
        {
            fprintf(stderr, "Missing argument.\n");
            exit(EXIT_FAILURE);
        }

        float value = strtof(av[2], NULL);
        if (strcmp(av[1], "-set") == 0)
        {
            options.operation = Set;
            options.value = value;
        }
        else if (strcmp(av[1], "-inc") == 0)
        {
            options.operation = Inc;
            options.value = value;
        }
        else if (strcmp(av[1], "-dec") == 0)
        {
            options.operation = Dec;
            options.value = value;
        }
        else
        {
            fprintf(stderr, "Invalid argument.\n");
            exit(EXIT_FAILURE);
        }
    }
    return options;
}

int get_value_file(char const * path)
{
    int fd;
    char buf[256];

    if ((fd = open(path, O_RDONLY)) == -1)
    {
        perror(path);
        exit(EXIT_FAILURE);
    }
    bzero(buf, 256);
    if (read(fd, buf, 256) == -1)
    {
        perror(path);
        exit(EXIT_FAILURE);
    }
    close(fd);
    return strtol(buf, NULL, 10);
}

void set_value_file(char const * path, int value)
{
    char buf[256];
    int fd;
    int len;

    if ((fd = open(path, O_WRONLY)) == -1)
    {
        perror(path);
        exit(EXIT_FAILURE);
    }
    len = sprintf(buf, "%d", value);
    if (write(fd, buf, len) == -1)
    {
        perror(path);
        exit(EXIT_FAILURE);
    }
    close(fd);
}

int get_cmd(struct br_info const * const info)
{
    printf("Current brightness: %0.02f%%\n",
           info->current * 1.0 / info->max * 100.0);
    return EXIT_SUCCESS;
}

int set_cmd(struct br_info const * const info, float value)
{
    int target;

    if (value <= 0)
        value = 0;
    else if (value > 100)
        value = 100;
    target = value / 100.0 * info->max;

    if (target <= 0)
        target = 1;

    printf("Setting brightness to %d\n", target);
    set_value_file(TARGET_BRIGHTNESS, target);
    return EXIT_SUCCESS;
}

int inc_cmd(struct br_info const * const info, float value)
{
    float ratio = info->current * 1.0 / info->max * 100.0;

    set_cmd(info, ratio + value);
    return EXIT_SUCCESS;
}

int dec_cmd(struct br_info const * const info, float value)
{
    float ratio = info->current * 1.0 / info->max * 100.0;

    set_cmd(info, ratio - value);
    return EXIT_SUCCESS;
}

void load_br_info(struct br_info * const info)
{
    info->current = get_value_file(CURRENT_BRIGHTNESS);
    info->max = get_value_file(MAX_BRIGHTNESS);
}


int main(int ac, char * const * const av)
{
    struct options options;
    struct br_info info;

    options = parse_options(ac, av);

    load_br_info(&info);
    int ret = EXIT_SUCCESS;
    switch (options.operation)
    {
    case Get:
        ret = get_cmd(&info);
        break ;
    case Set:
        ret = set_cmd(&info, options.value);
        break ;
    case Inc:
        ret = inc_cmd(&info, options.value);
        break ;
    case Dec:
        ret = dec_cmd(&info, options.value);
        break ;
    default:
        fprintf(stderr, "Invalid option\n");
        ret = EXIT_FAILURE;
        break;
    }

    return ret;
}
