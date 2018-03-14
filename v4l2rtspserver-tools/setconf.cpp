#include<stdio.h>
#include <getopt.h>
#include <cstdlib>
#include "sharedmem.h"


void usage(char *command)
{
    fprintf(stderr, "Usage %s -k KEY -v VALUE\n", command);
    fprintf(stderr, "Where k is in\n");
    fprintf(stderr, "\t'f' flip mode set to\n");
    fprintf(stderr, "\t\t'1' -> flip on\n");
    fprintf(stderr, "\t\t'0' -> flip off\n");

    fprintf(stderr, "\t'n' night mode set to\n");
    fprintf(stderr, "\t\t'1' -> night mode on\n");
    fprintf(stderr, "\t\t'0' -> night mode off\n");

    fprintf(stderr, "\t'b' set bitrate to VALUE\n");
    fprintf(stderr, "\t'o' OSD text set to VALUE\n");
    fprintf(stderr, "\t'c' OSD color set VALUE to\n");
    fprintf(stderr, "\t\t'0' for White\n");
    fprintf(stderr, "\t\t'1' for Black\n");
    fprintf(stderr, "\t\t'2' for Red\n");
    fprintf(stderr, "\t\t'3' for Green\n");
    fprintf(stderr, "\t\t'4 for Blue\n");
    fprintf(stderr, "\t\t'5' for Cyan\n");
    fprintf(stderr, "\t\t'6' for Yellow\n");
    fprintf(stderr, "\t\t'7' for Purple\n");

    fprintf(stderr, "\t'x' OSD position Y pos is set to VALUE\n");
    fprintf(stderr, "\t'p' OSD space between char is set to VALUE (can be negative)\n");
    fprintf(stderr, "\t'w' fixed text width (0 variable, 1 fixed)\n");

    fprintf(stderr, "\t'm' motion sensibility (0 to 4) -1 to deactivate motion\n");
    fprintf(stderr, "\t'z' display a red circle when motion detected (0 deactivated, 1 activated)\n");


}

int main(int argc, char *argv[]) {

    char key = 0;
    char *value;

    // Parse Arguments:
    int c;
    while ((c = getopt(argc, argv, "k:v:")) != -1) {
        switch (c) {
            case 'k':
                key = optarg[0];
                break;
            case 'v':
                value = optarg;
                break;
            default:
                printf("Invalid Argument %c\n", c);
                usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    SharedMem &mem = SharedMem::instance();
    shared_conf *conf = mem.getConfig();
    //printf("%d,%d\n", conf->nightmode, conf->flip);
    mem.readConfig();
    //printf("%d,%d\n", conf->nightmode, conf->flip);
    switch (key) {
        case 'f':
            conf->flip = atoi(value);
            break;
        case 'n':
            conf->nightmode = atoi(value);
            break;
        case 'b':
            conf->bitrate =  atoi(value);
            break;

        // OSD configuration
        case 'o':
            strcpy(conf->osdTimeDisplay,value);
            break;
        case 'c':
            conf->osdColor = atoi(value);
            break;
        case 's':
            conf->osdSize = atoi(value);
            break;
        case 'x':
            conf->osdPosY = atoi(value);
            break;
        case 'p':
            conf->osdSpace = atoi(value);
            break;
        case 'w':
            conf->osdFixedWidth = atoi(value)==0?false:true;
            break;
        // Motion configuration
        case 'm':
            conf->sensibility =  atoi(value);
            break;
        case 'z':
           conf->motionOSD =  atoi(value)==0?false:true;
           break;

    default:
        printf("Invalid Argument %c\n", key);
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }
    mem.setConfig();
    //mem.readConfig();
    //printf("%d,%d\n", conf->nightmode, conf->flip);

    return 0;


}

