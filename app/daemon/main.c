#include <stdio.h>

int main(int argc, char **argv)
{
    const char *config = "config/config.json";

    for (int i = 1; i < argc; ++i) {
        if ((argv[i][0] == '-') && (argv[i][1] == 'c') && argv[i + 1]) {
            config = argv[++i];
        }
    }

    printf("imx6ull-sense daemon scaffold\n");
    printf("config: %s\n", config);
    printf("next: implement capture_v4l2 -> jpeg_encoder -> http_server -> motion_detector\n");
    return 0;
}

