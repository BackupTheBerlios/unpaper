/* ---------------------------------------------------------------------------
unpaper 1.0 - written by Jens Gulden 2005                                   */

const char* README = 
"unpaper is a post-processing tool for scanned sheets of paper, especially for\n"
"book pages that have been scanned from previously created photocopies.\n"
"The main purpose is to make scanned book pages better readable on screen\n"
"after conversion to PDF. Additionally, unpaper might be useful to enhance\n"
"the quality of scanned pages before performing optical character recognition\n"
"(OCR).\n"
"\n"
"unpaper tries to clean scanned images by removing dark edges that appeared\n"
"through scanning or copying on areas outside the actual page content (e.g.\n"
"dark areas between the left-hand-side and the right-hand-side of a double-\n"
"sided book-page scan).\n"
"The program also tries to detect disaligned centering and rotation of pages\n"
"and will automatically straighten each page by rotating it to the correct\n"
"angle. This is called \"deskewing\".\n"
"Note that the automatic processing will sometimes fail. It is always a good\n"
"idea to manually control the results of unpaper and adjust the parameter\n"
"settings according to the requirements of the input. Each processing step can\n"
"also be disabled individually for each sheet.\n"
"\n"
"Input and output files can be in either .pbm or .pgm format, as also used by\n"
"the Linux scanning tools scanimage and scanadf.\n"
"Conversion to PDF can e.g. be achieved with the Linux tools pgm2tiff, tiffcp\n"
"and tiff2pdf.";

const char* COMPILE = 
"gcc -D TIMESTAMP=\"<yyyy-MM-dd HH:mm:ss>\" -lm -O3 -funroll-all-loops -o unpaper unpaper.c";
/* ------------------------------------------------------------------------ */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
 
const char* VERSION = "1.0";

#ifdef TIMESTAMP
const char* BUILD = TIMESTAMP;
#else
const char* BUILD = NULL;
#endif

const char* WELCOME = 
"unpaper %s - written by Jens Gulden 2005.\n"
"Licensed under the GNU General Public License, this comes with no warranty.\n";
              
const char* USAGE = 
"Usage: unpaper [options] <input-file> <output-file>\n\n"
"Filenames may contain a formatting placeholder starting with '%%' to insert a\n"
"page counter for multi-page processing. E.g.: 'scan%%03d.pbm' to process files\n"
"scan001.pbm, scan002.pbm, scan003.pbm etc.\n";

const char* OPTIONS = 
"-l --layout single|double            Set default layout options for a sheet:\n"
"                                     'single' - one page per sheet, oriented\n"
"                                                vertically without rotation\n"
"                                     'double' - two pages per sheet, rotated\n"
"                                                anti-clockwise (i.e. the top-\n"
"                                                sides of the pages are heading\n"
"                                                leftwards, and the pages are\n"
"                                                placed right-page above left-\n"
"                                                page on the unrotated sheet)\n"
"                                     Using this option automatically adjusts the\n"
"                                     --mask-point and --pre/post-rotation\n"
"                                     options.\n"
"-s --start-sheet <sheet>             Number of first sheet to process in multi-\n"
"                                     sheet mode. (default: 1)\n"
"-e --end-sheet <sheet>               Number of last sheet to process in multi-\n"
"                                     sheet mode. -1 indicates processing until\n"
"                                     no more input file file the corresponding\n"
"                                     page number is available (default: -1)\n"
"-# --sheet                           Optionally specifies which sheets to\n"
"     <sheet>{,<sheet>[-<sheet>]}     process in the range between start-sheet\n"
"                                     and end sheet.\n"
"-x --exclude                         Excludes sheets from processing in the\n"
"     <sheet>{,<sheet>[-<sheet>]}     range between start-sheet and end-sheet.\n"
"--pre-rotate -90|90                  Rotates the whole image clockwise (90) or\n"
"                                     or anti-clockwise (-90) before any other\n"
"                                     processing.\n"
"--post-rotate -90|90                 Rotates the whole image clockwise (90) or\n"
"                                     or anti-clockwise (-90) after any other\n"
"                                     processing.\n"
"-M --pre-mirror                      Mirror the image, after possible pre-\n"
"     [v[ertical]][,][h[orizontal]]   rotation. Either 'v' (for vertical\n"
"                                     mirroring), 'h' (for horizontal mirroring)\n"
"                                     or 'v,h' (for both) can be specified.\n"
"--post-mirror                        Mirror the image, after any other\n"
"  [v[ertical]][,][h[orizontal]]      processing except possible post-\n"
"                                     rotation.\n"
"--pre-wipe                           Manually wipe out an area before further\n"
"  <left>,<top>,<right>,<bottom>      processing. Any pixel in a wiped area\n"
"                                     will be set to white. Multiple areas to\n"
"                                     be wiped may be specified.\n"
"--post-wipe                          Manually wipe out an area after\n"
"  <left>,<top>,<right>,<bottom>      processing. Any pixel in a wiped area\n"
"                                     will be set to white. Multiple areas to\n"
"                                     be wiped may be specified.\n"
"--pre-border                         Clear the border-area of the sheet before\n"
"  <left>,<top>,<roght>,<bottom>      further processing. Any pixel inside the\n"
"                                     border will be set to white.\n"
"--post-border                        Clear the border-area after processing.\n"
"  <left>,<top>,<roght>,<bottom>      Any pixel inside the border will be set\n"
"                                     to white.\n"
"--pre-mask <x1>,<y1>,<x2>,<y2>       Specify masks to apply before any other\n"
"                                     processing. Any pixel outside a mask\n"
"                                     will be considered blank (white) pixels,\n"
"                                     unless another mask includes this pixel.\n"
"                                     Only pixels inside a mask will remain.\n"
"                                     Multiple masks may be specified. No\n"
"                                     deskewing will be applied to the masks\n"
"                                     specified by --pre-mask.\n"
"-bn --blackfilter-scan-direction     Directions in which to search for solidly\n"
"     [v[ertical]][,][h[orizontal]]   black areas. Either 'v' (for vertical\n"
"                                     mirroring), 'h' (for horizontal mirroring)\n"
"                                     of 'v,h' (for both) can be specified.\n"
"                                     (default: 'v,h')\n"
"-bs --blackfilter-scan-size          Width of virtual bar used for mask\n"
"      <size>|<h-size>,<v-size>       detection. Two values may be specified\n"
"                                     to individually set horizontal and vertical\n"
"                                     size. (default: 20,20)\n"
"-bd --blackfilter-scan-depth         Size of virtual bar used for black area\n"
"      <depth>|<h-depth,v-depth>      detection. (default: 500,500)\n"
"-bp --blackfilter-scan-step          Steps to move virtual bar for black area\n"
"      <step>|<h-step,v-step>         detection. (default: 5,5)\n"
"-bt --blackfilter-scan-threshold <t> Ratio of dark pixels above which a black\n"
"                                     area gets detected. (default: 0.95).\n"
"-bi --blackfilter-intensity <i>      Intensity with which to delete black areas.\n"
"                                     Larger values will leave less noise-pixels\n"
"                                     around former black areas, but may delete\n"
"                                     page content. (default: 20)\n"
"-ni --noisefilter-intensity <n>      Intensity with which to delete individual\n"
"                                     pixels or tiny clusters of pixels. Any\n"
"                                     cluster which only contains n dark pixels\n"
"                                     together will be deleted. (default: 4)\n"
"-ls --blurfilter-size                Size of blurfilter area to search for\n"
"      <size>|<h-size>,<v-size>       'lonely' clusters of pixels.\n"
"                                     (default: 100,100)\n"
"-lt --blurfilter-step                Size of 'blurring' steps in each\n"
"      <step>|<h-step>,<v-step>       direction. (default: 50,50)\n"
"-li --blurfilter-intensity <ratio>   Relative intensity with which to delete\n"
"                                     tiny clusters of pixels. Any blurred area\n"
"                                     which contains at most the ratio of dark\n"
"                                     pixels will be cleared. (default: 0.01)\n"
"-gs --grayfilter-size                Size of grayfilter mask to search for\n"
"      <size>|<h-size>,<v-size>       'gray-only' areas of pixels.\n"
"                                     (default: 50,50)\n"
"-gp --grayfilter-step                Size of steps moving the grayfilter mask\n"
"      <step>|<h-step>,<v-step>       in each direction. (default: 20,20)\n"
"-gt --grayfilter-threshold <ratio>   Relative intensity of grayness which is\n"
"                                     accepted before clearing the grayfilter\n"
"                                     mask in cases where no black pixel is\n"
"                                     found in the mask. (default: 0.5)\n"
"-p --mask-point <x>,<y>              Manually set starting point for masking.\n"
"                                     Multiple --mask-point parameters may be\n"
"                                     specified to process multiple pages on one\n"
"                                     sheet. Cannot be used in conjunction with\n"
"                                     --pages. (default: middle of image)\n"
"-m --mask <x1>,<y1>,<x2>,<y2>        Manually add a mask, in addition to masks\n"
"                                     automatically searched around the --point\n"
"                                     coordinates (unless --nomask is specified).\n"
"                                     Any pixel outside a mask will be considered\n"
"                                     a blank (white) pixel, unless another mask\n"
"                                     covers this pixel.\n"
"-mn --mask-scan-direction            Directions in which to search for inner mask\n"
"     [v[ertical]][,][h[orizontal]]   border. Either 'v' (for vertical\n"
"                                     scanning), 'h' (for horizontal scanning)\n"
"                                     of 'v,h' (for both) can be specified.\n"
"                                     (default: 'h' ('v' may cut paragraphs on\n"
"                                     single-page sheets))\n"
"-ms --mask-scan-size <size>|<h,v>    Width of virtual bar used for mask\n"
"                                     detection. Two values may be specified\n"
"                                     to individually set horizontal and vertical\n"
"                                     size. (default: 50,50)\n"
"-md --mask-scan-depth <dep>|<h,v>    Height of virtual bar used for mask\n"
"                                     detection. (default: -1,-1, using the whole\n"
"                                     width or height of the sheet)\n"
"-mp --mask-scan-step <step>|<h,v>    Steps to move virtual bar for mask\n"
"                                     detection. (default: 10,10)\n"
"-mt --mask-scan-threshold <t>|<h,v>  Ratio of dark pixels below which an edge\n"
"                                     gets detected, relative to max. blackness\n"
"                                     when counting from the starting coordinate\n"
"                                     heading towards one edge. (default: 0.1)\n"
"-mm --mask-scan-minimum <w>,<h>      Set minimum allowed size of an auto-\n"
"                                     detected mask. Masks detected below this\n"
"                                     size will be ignored and set to the size\n"
"                                     specified by mask-scan-maximum. (default:\n"
"                                     100,100)\n"
"-mM --mask-scan-maximum <w>,<h>      Set maximum allowed size of an auto-\n"
"                                     detected mask. Masks detected above this\n"
"                                     size will be shrunk to the maximum value,\n"
"                                     each direction individually. (default:\n"
"                                     sheet size, or page size derived from\n"
"                                     --layout option.\n"
"-mc --mask-color <color>             Set color / gray-scale value to overwrite\n"
"                                     pixels which are not covered by any\n"
"                                     detected mask. This may be useful for\n"
"                                     testing in order to visualize the effect\n"
"                                     of masking. (value: 0..255, default: 255)\n"
"-dn --deskew-scan-direction          Directions in which to scan for rotation.\n"
"     [v[ertical]][,][h[orizontal]]   Either 'h' (for horizontal scanning,\n"
"                                     starting at the left and right edges of a\n"
"                                     mask) or 'v' (for vertical scanning,\n"
"                                     starting at the top and bottom), or 'v,h'\n"
"                                     (for both) can be specified.\n"
"                                     (default: 'h' ('v' may be confused by\n"
"                                     headlines or footnotes))\n"
"-ds --deskew-scan-size <pixels>      Size of virtual line for rotation\n"
"                                     detection. (default: 1500)\n"
"-dd --deskew-scan-depth <ratio>      Amount of dark pixels to accumlate until\n"
"                                     scan is finished, relative to scan-bar\n"
"                                     size. (default: 0.66)\n"
"-dr --deskew-scan-range <degrees>    Range in which to search for rotation,\n"
"                                     from -degrees to +degrees rotation.\n"
"                                     (default: 2.0)\n"
"-dp --deskew-scan-step <degrees>     Steps between single rotation-range\n"
"                                     detections.\n"
"                                     Lower numbers lead to better results but\n"
"                                     slow down processing. (default: 0.1)\n"
"-dv --deskew-scan-deviation <dev>    Maximum deviation allowed between results\n"
"                                     from all detected edges to perform auto-\n"
"                                     rotating, else ignore. (default: 1.0)\n"
"-W --wipe                            Manually wipe out an area. Any pixel in\n"
"     <left>,<top>,<right>,<bottom>   a wiped area will be set to white.\n"
"                                     Multiple --wipe areas may be specified.\n"
"                                     This is applied after deskewing and\n"
"                                     before automatic border-scan.\n"
"-mw --middle-wipe                    If --layout is set to 'double', this\n"
"      <size>|<left>,<right>          may specify the size of a middle area to\n"
"                                     wipe out between the two pages on the\n"
"                                     sheet. This may be useful if the\n"
"                                     blackfilter fails to remove some black\n"
"                                     areas (which e.g. occur by photo-copying\n"
"                                     in the middle between two pages).\n"
"-B --border                          Manually add a border. Any pixel in the\n"
"     <left>,<top>,<right>,<bottom>   border area will be set to white. This is\n"
"                                     applied after deskewing and before\n"
"                                     automatic border-scan.\n"
"-Bn --border-scan-direction          Directions in which to search for outer\n"
"     [v[ertical]][,][h[orizontal]]   border. Either 'v' (for vertical\n"
"                                     scanning), 'h' (for horizontal scanning)\n"
"                                     of 'v,h' (for both) can be specified.\n"
"                                     (default: 'v')\n"
"-Bs --border-scan-size <size>|<h,v>  Width of virtual bar used for border\n"
"                                     detection. Two values may be specified\n"
"                                     to individually set horizontal and vertical\n"
"                                     size. (default: 5,5)\n"
"-Bp --border-scan-step <step>|<h,v>  Steps to move virtual bar for border\n"
"                                     detection. (default: 5,5)\n"
"-Bt --border-scan-threshold <t>      Absolute number of dark pixels covered by\n"
"                                     the border-scan mask above which a border\n"
"                                     is detected. (default: 5)\n"
"-w --white-threshold <threshold>     Brightness ratio above which a pixel is\n"
"                                     considered white. This is used when\n"
"                                     converting to black-and-white mode\n"
"                                     (default: 0.9)\n"
"-b --black-threshold <threshold>     Brightness ratio below which a pixel is\n"
"                                     considered black (non-gray). This is used\n"
"                                     by the gray-filter. (default: 0.5)\n"
"--no-blackfilter                     Disables black area scan. Individual sheet\n"
"  <sheet>{,<sheet>[-<sheet>]}        indices can be specified.\n"
"--no-noisefilter                     Disables noisefilter. Individual sheet\n"
"  <sheet>{,<sheet>[-<sheet>]}        indices can be specified.\n"
"--no-blurfilter                      Disables blurfilter. Individual sheet\n"
"  <sheet>{,<sheet>[-<sheet>]}        indices can be specified.\n"
"--no-mask-scan                       Disables auto-masking around the areas\n"
"  <sheet>{,<sheet>[-<sheet>]}        searched beginning from points specified\n"
"                                     by --point or auto-specified by --layout.\n"
"                                     Masks explicitly set by --mask will still\n"
"                                     have effect.\n"
"--no-mask-center                     Disables auto-centering of each mask.\n"
"  <sheet>{,<sheet>[-<sheet>]}        Auto-centering is performed by default\n"
"                                     if the --layout option has been set.\n"
"--no-deskew                          Disables auto-rotation to a straight\n"
"  <sheet>{,<sheet>[-<sheet>]}        alignment for individual sheets.\n"
"--no-wipe                            Disables explicitly wipe-areas.\n"
"  <sheet>{,<sheet>[-<sheet>]}        This means the effect of parameter\n"
"                                     --wipe is disabled individually per\n"
"                                     sheet.\n"
"--no-border                          Disables explicitly set borders.\n"
"  <sheet>{,<sheet>[-<sheet>]}        This means the effect of parameter\n"
"                                     --border is disabled individually per\n"
"                                     sheet.\n"
"--no-border-scan                     Disables automatic border-scanning at the\n"
"  <sheet>{,<sheet>[-<sheet>]}        edges of the sheet after most other\n"
"                                     processing has been done.\n"
"-n --no-processing                   Do not perform any processing on a sheet\n"
"     <sheet>{,<sheet>[-<sheet>]}     except pre/post rotating and mirroring,\n"
"                                     and file-type conversions on saving.\n"
"                                     This option has the same effect as setting\n"
"                                     --no-blackfilter, --no-noisefilter,\n"
"                                     --no-blurfilter, --no-grayfilter,\n"
"                                     --no-mask-scan, --no-deskew, --no-wipe,\n"
"                                     --no-mask-center, --no-border-scan and\n"
"                                     --no-border simultaneously.\n"
"--no-qpixels                         Disable qpixel-mode for deskewing\n"
"                                     (internally rotate a 4x bigger image and\n"
"                                     reshrink afterwards).\n"
"--no-multi-pages                     Disable multi-page processing even if the\n"
"                                     input filename contains a '%%' (usually\n"
"                                     indicating the start of a placeholder for\n"
"                                     the page counter).\n"
"-t --type pbm|pgm                    Output file type. (default: as input file)\n"
"-T --test-only                       Do not write any output. May be useful in\n"
"                                     combination with --verbose to get informa-\n"
"                                     tion about the input.\n"
"-q --quiet                           Quiet mode, no output at all.\n"
"-v --verbose                         Verbose output, more informational messages.\n"
"-vv                                  Even more verbose output, show parameter\n"
"                                     settings before processing.\n"
"-V --version                         Output version and build information.\n";
//-vvv --debug                        Undocumented.
//-vvvv --debug-save                  Undocumented.

const char* HELP = 
"Run 'unpaper --help' for usage information.\n";


/* --- preprocessor macros ------------------------------------------------ */
              
#define abs(value) ( (value) >=0 ? (value) : -(value) )



/* --- constants ---------------------------------------------------------- */
              
const MAX_POINTS = 100;
const MAX_MASKS = 100;
const MAX_ROTATION_SCAN_SIZE = 10000; // maximum pixel count of virtual line to detect rotation with
const MAX_MULTI_INDEX = 10000; // maximum pixel count of virtual line to detect rotation with
const WHITE = 255;
const GRAY = 128;
const BLACK = 0;



/* --- typedefs ----------------------------------------------------------- */

typedef enum {
    FALSE,
    TRUE
} BOOLEAN;

typedef enum {
    VERBOSE_QUIET = -1,
    VERBOSE_NONE = 0,
    VERBOSE_NORMAL = 1,
    VERBOSE_MORE = 2,
    VERBOSE_DEBUG = 3,
    VERBOSE_DEBUG_SAVE = 4
} VERBOSE_LEVEL;

typedef enum {
	X,
	Y,
	COORDINATES_COUNT
} COORDINATES;

typedef enum {
	WIDTH,
	HEIGHT,
	DIMENSIONS_COUNT
} DIMENSIONS;

typedef enum {
	HORIZONTAL,
	VERTICAL,
	DIRECTIONS_COUNT
} DIRECTIONS;

typedef enum {
	LEFT,
	TOP,
	RIGHT,
	BOTTOM,
	EDGES_COUNT
} EDGES;

typedef enum {
	PBM,
	PGM,
	PNM
} TYPES;

typedef enum {
    LAYOUT_NONE,
	LAYOUT_SINGLE,
	LAYOUT_DOUBLE,
	LAYOUTS_COUNT
} LAYOUTS;

typedef enum {
	BRIGHT,
	DARK,
	SHADINGS_COUNT
} SHADINGS;



/* --- global variable ---------------------------------------------------- */

VERBOSE_LEVEL verbose;



/****************************************************************************
 * tool functions                                                           *
 ****************************************************************************/


/* --- arithmetic tool functions ------------------------------------------ */

/**
 * Returns the quadratic square of a number.
 */
double sqr(double d) {
    return d*d;
}


/**
 * Converts degrees to radians.
 */
double degreesToRadians(double d) {
    return d * M_PI / 180.0;
}


/**
 * Converts radians to degrees.
 */
double radiansToDegrees(double r) {
    return r * 180.0 / M_PI;
}


/**
 * Limits an integer value to a maximum.
 */
void limit(int* i, int max) {
    if (*i > max) {
        *i = max;
    }
}


/* --- tool functions for parameter parsing and verbose output ------------ */

/**
 * Parses a parameter string on occurrences of 'vertical', 'horizontal' or both.
 */            
int parseDirections(char* s) {
    int dir = 0;
    if (strchr(s, 'h') != 0) { // (luckily there is no 'h' in 'vertical'...)
        dir = 1<<HORIZONTAL;
    }
    if (strchr(s, 'v') != 0) { // (luckily there is no 'v' in 'horizontal'...)
        dir |= 1<<VERTICAL;
    }
    return dir;
}


/**
 * Prints whether directions are vertical, horizontal, or both.
 */            
void printDirections(int d) {
    if ((d & 1<<VERTICAL) != 0) {
        printf("vertical ");
    }
    if ((d & 1<<HORIZONTAL) != 0) {
        printf("horizontal");
    }
    printf("\n");
}


/**
 * Parses either a single integer string, of a pair of two integers seperated
 * by a comma.
 */            
void parseInts(char* s, int i[2]) {
    i[0] = -1;
    i[1] = -1;
    sscanf(s, "%i,%i", &i[0], &i[1]);
    if (i[1]==-1) {
        i[1] = i[0]; // if second value is unset, copy first one into
    }
}


/**
 * Outputs either a single integer value, of a pair of two integers seerated
 * by a comma.
 */            
void printInts(int i[2]) {
    printf("[%i,%i]\n", i[0], i[1]);
}


/**
 * Parses either a single float string, of a pair of two floats seperated
 * by a comma.
 */            
void parseFloats(char* s, float f[2]) {
    f[0] = -1.0;
    f[1] = -1.0;
    sscanf(s, "%f,%f", &f[0], &f[1]);
    if (f[1]==-1.0) {
        f[1] = f[0]; // if second value is unset, copy first one into
    }
}


/**
 * Outputs either a single float value, of a pair of two floats seperated by a
 * comma.
 */            
void printFloats(float f[2]) {
    printf("[%f,%f]\n", f[0], f[1]);
}


/**
 * Parses a string at argv[*i] argument consisting of comma-concatenated 
 * integers. The string may also be of a different format, in which case
 * *i remains unchanged and *multiIndexCount is set to -1.
 * @see isInMultiIndex(..)
 */
void parseMultiIndex(int* i, char* argv[], int multiIndex[], int* multiIndexCount) {
    char s1[MAX_MULTI_INDEX*5]; // buffer
    char s2[MAX_MULTI_INDEX*5]; // buffer
    char c;
    int index;
    int j;
    
    (*i)++;
    *multiIndexCount = 0;
    if (argv[*i][0] != '-') { // not another option directly following
        strcpy(s1, argv[*i]); // argv[*i] -> s1
        do {
            index = -1;
            s2[0] = (char)0; // set to empty
            sscanf(s1, "%i%c%s", &index, &c, s2);
            if (index != -1) {
                multiIndex[(*multiIndexCount)++] = index;
                if (c=='-') { // range is specified: get range end
                    strcpy(s1, s2); // s2 -> s1
                    sscanf(s1, "%i,%s", &index, s2);
                    for (j = multiIndex[(*multiIndexCount)-1]+1; j <= index; j++) {
                        multiIndex[(*multiIndexCount)++] = j;
                    }
                }
            } else {
                // string is not correctly parseable: break without inreasing *i (string may be e.g. input-filename)
                *multiIndexCount = -1; // disable all
                (*i)--;
                return; // exit here
            }
            strcpy(s1, s2); // s2 -> s1
        } while ((*multiIndexCount < MAX_MULTI_INDEX) && (strlen(s1) > 0));
    } else { // no explicit list of sheet-numbers given
        *multiIndexCount = -1; // disable all
        (*i)--;
        return;
    }
}


/**
 * Tests whether an integer is included in the array of integers given as multiIndex.
 * If multiIndexCount is -1, each possible integer is considered to be in the
 * multiIndex list.
 * @see parseMultiIndex(..)
 */
BOOLEAN isInMultiIndex(int index, int multiIndex[MAX_MULTI_INDEX], int multiIndexCount) {
    int i;
    
    if (multiIndexCount == -1) {
        return TRUE; // all
    } else {
        for (i = 0; i < multiIndexCount; i++) {
            if (index == multiIndex[i]) {
                return TRUE; // found in list
            }
        }
        return FALSE; // not found in list
    }
}


/**
 * Tests whether 'index' is either part of multiIndex or excludeIndex.
 * (Throughout the application, excludeIndex generalizes several individual 
 * multi-indices: if an entry is part of excludeIndex, it is treated as being
 * an entry of all other multiIndices, too.)
 */
BOOLEAN isExcluded(int index, int multiIndex[MAX_MULTI_INDEX], int multiIndexCount, int excludeIndex[MAX_MULTI_INDEX], int excludeIndexCount) {
    return ( (isInMultiIndex(index, excludeIndex, excludeIndexCount) == TRUE) || (isInMultiIndex(index, multiIndex, multiIndexCount) == TRUE) );
}


/**
 * Outputs all entries in an array of integer to the console.
 */
void printMultiIndex(int multiIndex[MAX_MULTI_INDEX], int multiIndexCount) {
    int i;
    
    if (multiIndexCount == -1) {
        printf("all");
    } else if (multiIndexCount == 0) {
        printf("none");
    } else {
        for (i = 0; i < multiIndexCount; i++) {
            printf("%i", multiIndex[i]);
            if (i < multiIndexCount-1) {
                printf(",");
            }
        }
    }
    printf("\n");
}


/* --- tool functions for image handling ---------------------------------- */

/**
 * Sets the color/grayscale value of a single pixel.
 * @return TRUE if the pixel has been changed, FALSE if the original color was the one to set
 */ 
BOOLEAN setPixel(int pixel, int x, int y, unsigned char* buffer, int w, int h, int type) {
    unsigned char* p;
    if (x<0||x>=w||y<0||y>=h) {
        //nop
    } else {
        p = &buffer[y*w+x];
        if (*p != (unsigned char)pixel) {
            *p = (unsigned char)pixel;
            return TRUE;
        } else {
            return FALSE;
        }
    }
}


/**
 * Returns the color/grayscale value of a single pixel.
 */ 
int getPixel(int x, int y, unsigned char* buf, int w, int h, int type) {
    if (x<0 || x>=w || y<0 || y>=h) {
        return WHITE;
    } else {
        return (unsigned char)buf[(y * w) + x];
    }
}


/**
 * Copies one area of an image into another. Color conversions are possible,
 * if getPixel/setPixel supports different types.
 */
void copyBuffer(int x, int y, int w, int h, unsigned char* buf, int wBuf, int hBuf, int typeBuf,
                int toX, int toY, unsigned char* target, int wTarget, int hTarget, int typeTarget) {
    int row;
    int col;
    int pixel;
    // naive but generic implementation
    for (row = 0; row < h; row++) {
        for (col = 0; col < w; col++) {
            pixel = getPixel(x+col, y+row, buf, wBuf, hBuf, typeBuf);
            setPixel(pixel, toX+col, toY+row, target, wTarget, hTarget, typeTarget);
        }
    }
}


/**
 * Returns the average brightness of a rectagular area.
 */
int brightnessRect(int x1, int y1, int x2, int y2, unsigned char* buf, int w, int h, int type) {
    int x;
    int y;
    int pixel;
    int total;
    int count;
    total = 0;
    count = (x2-x1+1)*(y2-y1+1);
    for (x = x1; x <= x2; x++) {
        for (y = y1; y <= y2; y++) {
            pixel = getPixel(x, y, buf, w, h, type);
            total += pixel;
        }
    }
    return total / count;
}


/**
 * Counts the number of pixels in a rectangular area whose color/grayscale
 * values ranges between minColor and maxColor. Optionally, the area can get
 * cleared with white color while counting.
 */
int countPixelsRect(int left, int top, int right, int bottom, int minColor, int maxColor, BOOLEAN clear, unsigned char* buf, int w, int h, int type) {
    int x;
    int y;
    int pixel;
    int count;
    
    count = 0;
    for (y = top; y <= bottom; y++) {
        for (x = left; x <= right; x++) {
            pixel = getPixel(x, y, buf, w, h, type);
            if ((pixel>=minColor) && (pixel <= maxColor)) {
                if (clear==TRUE) {
                    setPixel(WHITE, x, y, buf, w, h, type);
                }
                count++;
            }
        }
    }
    return count;
}


/**
 * Fills a rectangular area of pixels with a specified color.
 */
int fillRect(int color, int left, int top, int right, int bottom, unsigned char* buf, int w, int h, int type) {
    int x;
    int y;
    int count;
    
    count = 0;
    for (y = top; y <= bottom; y++) {
        for (x = left; x <= right; x++) {
            if (setPixel(color, x, y, buf, w, h, type)) {
                count++;
            }
        }
    }
    return count;
}


/**
 * Counts the number of dark pixels around the pixel at (x,y), who have a
 * square-metric distance of 'level' to the (x,y) (thus, testing the values
 * of 9 pixels if level==1, 16 pixels if level==2 and so on).
 * Optionally, the pixels can get cleared after counting.
 */
int countPixelNeighborsLevel(int x, int y, BOOLEAN clear, int level, int whiteMin, unsigned char* buf, int w, int h, int type) {
    int xx;
    int yy;
    int count;
    int pixel;
    
    count = 0;
    for (yy = y-level; yy <= y+level; yy++) {
        for (xx = x-level; xx <= x+level; xx++) {
            if (abs(xx-x)==level || abs(yy-y)==level) {
                pixel = getPixel(xx, yy, buf, w, h, type);
                if (pixel < whiteMin) {
                    if (clear==TRUE) {
                        setPixel(WHITE, xx, yy, buf, w, h, type);
                    }
                    count++;
                }
            }
        }    
    }
    return count;
}


/**
 * Count all dark pixels in the distance 0..intensity that are directly
 * reachable from the dark pixel at (x,y), without having to cross bright
 * pixels.
 */
int countPixelNeighbors(int x, int y, int intensity, int whiteMin, unsigned char* buf, int w, int h, int type) {
    int level;
    int count;
    int lCount;
    
    count = 1; // assume self as set
    lCount = -1;
    for (level = 1; (lCount != 0) && (level <= intensity); level++) { // can break when on level is completely zero
        lCount = countPixelNeighborsLevel(x, y, FALSE, level, whiteMin, buf, w, h, type);
        count += lCount;
    }
    return count;
}


/**
 * Clears all dark pixels that are directly reachable from the dark pixel at
 * (x,y). This should be called only if it has previously been detected that
 * the amount of pixels to clear will be reasonable small.
 */
void clearPixelNeighbors(int x, int y, int whiteMin, unsigned char* buf, int w, int h, int type) {
    int level;
    int lCount;

    setPixel(WHITE, x, y, buf, w, h, type);    
    lCount = -1;
    for (level = 1; lCount != 0; level++) { // lCount will become 0, otherwise countPixelNeighbors() would previously have delivered a bigger value (and this here would not have been called)
        lCount = countPixelNeighborsLevel(x, y, TRUE, level, whiteMin, buf, w, h, type);
    }
}


/**
 * Flood-fill an area of pixels.
 * (Declaration of header for indirect recursive calls.)
 */
void floodFill(int x, int y, int color, int maskMin, int maskMax, int intensity, unsigned char* buf, int w, int h, int type);


/**
 * Solidly fills a line of pixels heading towards a specified direction
 * until color-changes in the pixels to overwrite exceed the 'intensity'
 * parameter.
 * @param stepX either -1 or 1, if stepY is 0, else 0
 * @param stepY either -1 or 1, if stepX is 0, else 0
 */
int fillLine(int x, int y, int stepX, int stepY, int color, int maskMin, int maskMax, int intensity, unsigned char* buf, int w, int h, int type) {
    int pixel;
    int distance;
    int intensityCount;

    distance = 0;
    intensityCount = 1; // first pixel must match, otherwise directly exit
    while (1==1) { // !
        x += stepX;
        y += stepY;
        pixel = getPixel(x, y, buf, w, h, type);
        if ((pixel>=maskMin) && (pixel<=maskMax)) {
            intensityCount = intensity; // reset counter
        } else {
            intensityCount--; // allow maximum of 'intensity' pixels to be bright, until stop
        }
        if ((intensityCount > 0) && (x>=0) && (x<w) && (y>=0) && (y<h)) {
            setPixel(color, x, y, buf, w, h, type);
            distance++;
        } else {
            return distance; // exit here
        }
    }
}


/**
* Start flood-filling around the edges of a line which has previously been
* filled using fillLine(). Here, the flood-fill algorithm performs its
* indirect recursion.
 * @param stepX either -1 or 1, if stepY is 0, else 0
 * @param stepY either -1 or 1, if stepX is 0, else 0
 * @see fillLine()
 */
void floodFillAroundLine(int x, int y, int stepX, int stepY, int distance, int color, int maskMin, int maskMax, int intensity, unsigned char* buf, int w, int h, int type) {
    int d;
    
    for (d = 0; d < distance; d++) {
        if (stepX != 0) {
            x += stepX;
            floodFill(x, y + 1, color, maskMin, maskMax, intensity, buf, w, h, type); // indirect recursion
            floodFill(x, y - 1, color, maskMin, maskMax, intensity, buf, w, h, type); // indirect recursion
        } else { // stepY != 0
            y += stepY;
            floodFill(x + 1, y, color, maskMin, maskMax, intensity, buf, w, h, type); // indirect recursion
            floodFill(x - 1, y, color, maskMin, maskMax, intensity, buf, w, h, type); // indirect recursion
        }
    }
}


/**
 * Flood-fill an area of pixels. (Naive implementation, highly optimizable.)
 * @see previous header-declaration to enable indirect recursive calls
 */
void floodFill(int x, int y, int color, int maskMin, int maskMax, int intensity, unsigned char* buf, int w, int h, int type) {
    int left;
    int top;
    int right;
    int bottom;
    int pixel;
    
    // is current pixel to be filled?
    pixel = getPixel(x, y, buf, w, h, type);
    if ((pixel>=maskMin) && (pixel<=maskMax)) {
        // first, fill a 'cross' (both vertical, horizontal line)
        setPixel(color, x, y, buf, w, h, type);
        left = fillLine(x, y, -1, 0, color, maskMin, maskMax, intensity, buf, w, h, type);
        top = fillLine(x, y, 0, -1, color, maskMin, maskMax, intensity, buf, w, h, type);
        right = fillLine(x, y, 1, 0, color, maskMin, maskMax, intensity, buf, w, h, type);
        bottom = fillLine(x, y, 0, 1, color, maskMin, maskMax, intensity, buf, w, h, type);
        // now recurse on each neighborhood-pixel of the cross (most recursions will immediately return)
        floodFillAroundLine(x, y, -1, 0, left, color, maskMin, maskMax, intensity, buf, w, h, type);
        floodFillAroundLine(x, y, 0, -1, top, color, maskMin, maskMax, intensity, buf, w, h, type);
        floodFillAroundLine(x, y, 1, 0, right, color, maskMin, maskMax, intensity, buf, w, h, type);
        floodFillAroundLine(x, y, 0, 1, bottom, color, maskMin, maskMax, intensity, buf, w, h, type);
    }
}



/* --- tool function for file handling ------------------------------------ */

/**
 * Tests if a file exists.
 */
BOOLEAN fileExists(char* filename) {
    FILE *f;
    f = fopen(filename,"r");
    if (f == NULL) {
        return FALSE;
    } else {
        fclose(f);
        return TRUE;
    }
}


/**
 * Loads image data from a file in pgm or pbm format.
 * @param filename name of file to load
 * @param buffer returns pointer to buffer with loaded image data
 * @param width returns width of the loaded image
 * @param height returns height of the loaded image
 * @param type returns the type of the loaded image
 * @return TRUE on success, FALSE on failure
 */
BOOLEAN loadImage(char* filename, unsigned char** buffer, int* width, int* height, int* type) {
    FILE *f;
    int fileSize;
    int bytesPerLine;
    char magic[100];
    char word[255];
    char c;
    int maxColorIndex;
    int inputSize;
    int inputSizeFile;
    int read;
    unsigned char* buffer2;
    int lineOffsetInput;
    int lineOffsetOutput;
    int x;
    int y;
    int b;
    int off;
    int bits;
    int bit;
    int pixel;

    if (verbose>=VERBOSE_MORE) {
        printf("loading file %s.\n", filename);
    }

    // open input file
    f = fopen(filename, "rb");
    if (f==NULL) {
        printf("*** error: Unable to open file %s.\n", filename);
        return FALSE;
    }

    // get file size
    fseek(f, 0, SEEK_END); // to end
    fileSize = ftell(f);
    rewind(f); // back to start

    // determine type    
    fscanf(f, "%s\n", magic);
    if (strcmp(magic, "P4")==0) {
        *type = PBM;
    } else if (strcmp(magic, "P5")==0) {
        *type = PGM;
    } else {
        printf("*** error: Input file format using magic '%s' is unknown.\n", magic);
        return FALSE;
    }

    fscanf(f, "%s", word);
    while (word[0]=='#') { // skip comment lines
        do {
            fscanf(f, "%c", &c);
        } while ((feof(f)==0)&&(c!='\n'));
        fscanf(f, "%s", word);
    }
    
    // now reached width/height pair as decimal ascii
    sscanf(word, "%i", width);
    fscanf(f, "%i", height);
    
    if (*type==PBM) {
        bytesPerLine = (*width + 7) >> 3; // / 8;
        
    } else { // PGM
        fscanf(f, "%s", word);
        while (word[0]=='#') { // skip comment lines
            do {
                fscanf(f, "%c", &c);
            } while ((feof(f)==0)&&(c!='\n'));
            fscanf(f, "%s", word);
        }
        sscanf(word, "%i", &maxColorIndex);
        if (maxColorIndex > 255) {
            printf("*** error: pgm files with more than 255 colors are not supported.\n");
            return FALSE;
        }
        bytesPerLine = *width;
    }

    // read binary image data
    inputSizeFile = fileSize - ftell(f);
    inputSize = bytesPerLine * (*height);

    *buffer = (char*)malloc(inputSize);
    read = fread(*buffer, 1, inputSize, f);
    if (read != inputSize) {
        printf("*** error: Only %i out of %i could be read.\n", read, inputSize);
        return FALSE;
    }
    
    if (*type==PBM) { // internally convert to pgm
        buffer2 = (char*)malloc((*width) * (*height));
        lineOffsetInput = 0;
        lineOffsetOutput = 0;
        for (y = 0; y < (*height); y++) {
            for (x = 0; x < (*width); x++) {
                b = x >> 3;  // x / 8;
                off = x & 7; // x % 8;
                bit = 128>>off;
                bits = (*buffer)[lineOffsetInput+b];
                bits &= bit;
                if (bits==0) { // 0: white pixel
                    pixel = 0xff;
                } else {
                    pixel = 0x00;
                }
                buffer2[lineOffsetOutput+x] = pixel; // set as whole byte
            }
            lineOffsetInput += bytesPerLine;
            lineOffsetOutput += (*width);
        }
        free(*buffer);
        *buffer = buffer2;
    }
    fclose(f);
    return TRUE;
}


/**
 * Saves image data to a file in pgm or pbm format.
 * @param filename name of file to save
 * @param buffer pointer to buffer with image data to save
 * @param width width of the image to save
 * @param height height of the image to save
 * @param type type of the image to save
 * @return TRUE on success, FALSE on failure
 */
BOOLEAN saveImage(char* filename, unsigned char* buffer, int width, int height, int type, float blackThreshold) {
    unsigned char* buffer2;
    int bytesPerLine;
    int outputSize;
    int lineOffsetInput;
    int lineOffsetOutput;
    int x;
    int y;
    int pixel;
    int b;
    int off;
    unsigned char bit;
    unsigned char val;
    int i;
    char* outputMagic;
    FILE* outputFile;
    int blackThresholdAbs;
    BOOLEAN result;

    if (verbose>=VERBOSE_MORE) {
        printf("saving file %s.\n", filename);
    }

    result = TRUE;
    if (type==PBM) { // convert to pbm
        blackThresholdAbs = WHITE * (1.0 - blackThreshold);
        bytesPerLine = (width + 7) >> 3; // / 8;
        outputSize = bytesPerLine * height;
        buffer2 = (char*)malloc(outputSize);
        for (i = 0; i < outputSize; i++) {
            buffer2[i] = 0; // clear each bit (edge-bits on the right may stay unused and should be blank)
        }
        lineOffsetInput = 0;
        lineOffsetOutput = 0;
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++) {
                pixel = buffer[lineOffsetInput+x];
                b = x >> 3; // / 8;
                off = x & 7; // % 8;
                bit = 128>>off;
                val = buffer2[lineOffsetOutput+b];
                if (pixel < blackThresholdAbs) { // dark
                    val |= bit; // set bit to one: black
                } else { // bright
                    val &= (~bit); // set bit to zero: white
                }
                buffer2[lineOffsetOutput+b] = val;
            }
            lineOffsetOutput += bytesPerLine;
            lineOffsetInput += width;
        }
        buffer = buffer2;
        outputMagic = "P4";
    } else { // PGM output
        outputSize = width * height;
        outputMagic = "P5";
        buffer2 = NULL;
    }

    // write to file
    outputFile = fopen(filename, "wb");
    if (outputFile != 0) {
        fprintf(outputFile, "%s\n", outputMagic);
        fprintf(outputFile, "# generated by unpaper\n");
        fprintf(outputFile, "%i %u\n", width, height);
        if (type==PGM) {
            fprintf(outputFile, "255\n"); // bg color (?)
        }
        fwrite(buffer, 1, outputSize, outputFile);
        fprintf(outputFile, "%c", 0); // zero-termination byte
    	fclose(outputFile);
    } else {
        printf("*** error: Cannot open output file '%s'.\n", filename);
        result = FALSE;
    }
    if (buffer2 != NULL) {
        free(buffer2);
    }
    return result;
}    


/**
 * Saves the image if full debugging mode is enabled.
 */
void saveDebug(char* filename, unsigned char* buffer, int width, int height) {
    if (verbose >= VERBOSE_DEBUG_SAVE) {
        saveImage(filename, buffer, width, height, PGM, 0.5); // 0.5 is a dummy, not used because PGM type
    }
}



/****************************************************************************
 * conceptual functions                                                     *
 ****************************************************************************/


/* --- deskewing ---------------------------------------------------------- */

/**
 * Returns the maximum peak value that occurs when shifting a rotated virtual line above the image,
 * starting from one edge of an area and moving towards the middle point of the area.
 * The peak value is calulated by the absolute difference in the average blackness of pixels that occurs between two single shifting steps.
 * @param m Steigungsfaktor of the virtually shifted (m=tan(angle)). Mind that this is negative for negative radians.
 */
int detectEdgeRotationPeak(double m, int deskewScanSize, float deskewScanDepth, float deskewScanThreshold, int shiftX, int shiftY, int left, int top, int right, int bottom, unsigned char* buf, int w, int h, int type) {
    int width;
    int height;
    int mid;
    int half;
    int sideOffset;
    int outerOffset;
    double X; // unrounded coordinates
    double Y;
    double stepX;
    double stepY;
    int x[MAX_ROTATION_SCAN_SIZE];
    int y[MAX_ROTATION_SCAN_SIZE];
    int xx;
    int yy;
    int lineStep;
    int depth;
    int pixel;
    int blackness;
    int lastBlackness;
    int diff;
    int maxDiff;
    int maxBlacknessAbs;
    int maxDepth;
    int accumulatedBlackness;
        
    width = right-left+1;
    height = bottom-top+1;    
    outerOffset = (int)(abs(m) * half);
    maxBlacknessAbs = (int) 255 * deskewScanSize * deskewScanDepth;
    
    if (shiftY==0) { // horizontal detection
        if (deskewScanSize == -1) {
            deskewScanSize = height;
        }
        limit(&deskewScanSize, MAX_ROTATION_SCAN_SIZE);
        limit(&deskewScanSize, height);

        maxDepth = width/2;
        half = deskewScanSize/2;
        mid = height/2;
        sideOffset = shiftX > 0 ? left-outerOffset : right+outerOffset;
        X = sideOffset + half * m;
        Y = top + mid - half;
        stepX = -m;
        stepY = 1.0;
    } else { // vertical detection
        if (deskewScanSize == -1) {
            deskewScanSize = width;
        }
        limit(&deskewScanSize, MAX_ROTATION_SCAN_SIZE);
        limit(&deskewScanSize, width);
        maxDepth = height/2;
        half = deskewScanSize/2;
        mid = width/2;
        sideOffset = shiftY > 0 ? top-outerOffset : bottom+outerOffset;
        X = left + mid - half;
        Y = sideOffset - (half * m);
        stepX = 1.0;
        stepY = -m; // (line goes upwards for negative degrees)
    }
    
    // fill buffer with coordinates for rotated line in first unshifted position
    for (lineStep = 0; lineStep < deskewScanSize; lineStep++) {
        x[lineStep] = (int)X;
        y[lineStep] = (int)Y;
        X += stepX;
        Y += stepY;
    }
    
    // now scan for edge, modify coordinates in buffer to shift line into search direction (towards the middle point of the area)
    // stop either when detectMaxDepth steps are shifted, or when diff falls back to less than detectThreshold*maxDiff
    lastBlackness = 0;
    diff = 0;
    maxDiff = 0;
    accumulatedBlackness = 0;
    for (depth = 0; (accumulatedBlackness < maxBlacknessAbs) && (depth < maxDepth) ; depth++) { // && !(diff < maxDiff*deskewScanThreshold)
        // calculate blackness of virtual line
        blackness = 0;
        for (lineStep = 0; lineStep < deskewScanSize; lineStep++) {
            xx = x[lineStep];
            x[lineStep] += shiftX;
            yy = y[lineStep];
            y[lineStep] += shiftY;
            if ((xx >= left) && (xx <= right) && (yy >= top) && (yy <= bottom)) {
                pixel = getPixel(xx, yy, buf, w, h, type);
                blackness += (255 - pixel);
            }
        }
        diff = blackness - lastBlackness;
        lastBlackness = blackness;
        if (diff >= maxDiff) {
            maxDiff = diff;
        }
        accumulatedBlackness += blackness;
    }
    if (depth < maxDepth) { // has not terminated only because middle was reached
        return maxDiff;
    } else {
        return 0;
    }
}


/**
 * Detects rotation at one edge of the area specified by left, top, right, bottom.
 * Which of the four edges to take depends on whether shiftX or shiftY is non-zero,
 * and what sign this shifting value has.
 */
double detectEdgeRotation(float deskewScanRange, float deskewScanStep, int deskewScanSize, float deskewScanDepth, float deskewScanThreshold, int shiftX, int shiftY, int left, int top, int right, int bottom, unsigned char* buf, int w, int h, int type) {
    // either shiftX or shiftY is 0, the other value is -i|+i
    // depending on shiftX/shiftY the start edge for shifting is determined
    double rangeRad;
    double stepRad;
    double rotation;
    int peak;
    int maxPeak;
    double detectedRotation;
    double m;

    rangeRad = degreesToRadians((double)deskewScanRange);
    stepRad = degreesToRadians((double)deskewScanStep);
    detectedRotation = 0.0;
    maxPeak = 0;    
    // iteratively increase test angle,  alterating between +/- sign while increasing absolute value
    for (rotation = 0.0; rotation <= rangeRad; rotation = (rotation>=0.0) ? -(rotation + stepRad) : -rotation ) {    
        m = tan(rotation);
        peak = detectEdgeRotationPeak(m, deskewScanSize, deskewScanDepth, deskewScanThreshold, shiftX, shiftY, left, top, right, bottom, buf, w, h, type);
        if (peak > maxPeak) {
            detectedRotation = rotation;
            maxPeak = peak;
        }
    }
    return radiansToDegrees(detectedRotation);
}


/**
 * Detect rotation of a whole area. 
 * Angles between -deskewScanRange and +deskewScanRange are scanned, at either the
 * horizontal or vertical edges of the area specified by left, top, right, bottom.
 */
double detectRotation(int deskewScanDirections, int deskewScanRange, float deskewScanStep, int deskewScanSize, float deskewScanDepth, float deskewScanThreshold, float deskewScanDeviation, int left, int top, int right, int bottom, unsigned char* buf, int w, int h, int type) {
    double rotation[4];
    int count;
    double total;
    double average;
    double deviation;
    int i;
    
    count = 0;
    if ((deskewScanDirections & 1<<HORIZONTAL) != 0) {
        // left
        rotation[count] = detectEdgeRotation(deskewScanRange, deskewScanStep, deskewScanSize, deskewScanDepth, deskewScanThreshold, 1, 0, left, top, right, bottom, buf, w, h, type);
        if (verbose >= VERBOSE_DEBUG) {
            printf("detected rotation left: [%i,%i,%i,%i]: %f\n", left,top,right,bottom, rotation[count]);
        }
        count++;
        // right
        rotation[count] = detectEdgeRotation(deskewScanRange, deskewScanStep, deskewScanSize, deskewScanDepth, deskewScanThreshold, -1, 0, left, top, right, bottom, buf, w, h, type);
        if (verbose >= VERBOSE_DEBUG) {
            printf("detected rotation right: [%i,%i,%i,%i]: %f\n", left,top,right,bottom, rotation[count]);
        }
        count++;
    }
    if ((deskewScanDirections & 1<<VERTICAL) != 0) {
        // top
        rotation[count] = detectEdgeRotation(deskewScanRange, deskewScanStep, deskewScanSize, deskewScanDepth, deskewScanThreshold, 0, 1, left, top, right, bottom, buf, w, h, type);
        if (verbose >= VERBOSE_DEBUG) {
            printf("detected rotation top: [%i,%i,%i,%i]: %f\n", left,top,right,bottom, rotation[count]);
        }
        count++;
        // bottom
        rotation[count] = detectEdgeRotation(deskewScanRange, deskewScanStep, deskewScanSize, deskewScanDepth, deskewScanThreshold, 0, -1, left, top, right, bottom, buf, w, h, type);
        if (verbose >= VERBOSE_DEBUG) {
            printf("detected rotation bottom: [%i,%i,%i,%i]: %f\n", left,top,right,bottom, rotation[count]);
        }
        count++;
    }
    
    total = 0.0;
    for (i = 0; i < count; i++) {
        total += rotation[i];
    }
    average = total / count;
    total = 0.0;
    for (i = 0; i < count; i++) {
        total += sqr(rotation[i]-average);
    }
    deviation = sqrt(total);
    if (verbose >= VERBOSE_DEBUG) {
        printf("rotation average: %f  deviation: %f  rotation-scan-deviation (maximum): %f  [%i,%i,%i,%i]\n", average, deviation, deskewScanDeviation, left,top,right,bottom);
    }
    if (deviation <= deskewScanDeviation) {
        return average;
    } else {
        if (verbose >= VERBOSE_NORMAL) {
            printf("out of deviation range - NO ROTATING\n");
        }
        return 0.0;
    }
}


/**
 * Rotates a whole image buffer by the specified radians, around its middle-point.
 * Usually, the buffer should have been converted to a qpixels-representation before, to increase quality.
 * (To rotate parts of an image, extract the part with copyBuffer, rotate, and re-paste with copyBuffer.)
 */
void rotate(double radians, unsigned char* buf, unsigned char* target, int w, int h, int type) {
    int x;
    int y;
    int midX;
    int midY;
    int dX;
    int dY;
    double hyp;
    double alpha;
    double alphaNew;
    int origX;
    int origY;
    int pixel;
    double sin;
    double cos;
    int i;
    
    sincos(radians, &sin, &cos);
    midX = w/2;
    midY = h/2;    
    // step through all pixels of the target image
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            // find rotated point in original image
            dX = x - midX;
            dY = y - midY;
            hyp = sqrt(dX*dX + dY*dY);
            if (y!=midY) {
                alpha = atan((double)dX/-dY);
            } else { // dY==0, avoid division by 0
                alpha = M_PI/2;
            }
            if (dY > 0) { // adopt angle to quadrant (atan() can only return -90..90)
                alpha -= M_PI;
            }
            alphaNew = alpha -radians;
            sincos(alphaNew, &sin, &cos);
            origX = midX + (int)(hyp * sin);
            origY = midY - (int)(hyp * cos);
            // set point value
            pixel = getPixel(origX, origY, buf, w, h, type);
            setPixel(pixel, x, y, target, w, h, type);
        }
    }
}


/**
 * Converts an image buffer to a qpixel-representation, i.e. enlarge the whole
 * whole image both horizontally and vertically by factor 2 (leading to a
 * factor 4 increase in total).
 * qpixelBuf must have been allocated before with 4-times amount of memory as
 * buf.
 */
void convertToQpixels(char* buf, int w, int h, int type, unsigned char* qpixelBuf) {
    int x;
    int y;
    int xx;
    int yy;
    int pixel;
    int w2;
    int h2;
    
    w2 = w*2;
    h2 = h*2;
    yy = 0;
    for (y=0; y<h; y++) {
        xx = 0;
        for (x=0; x<w; x++) {
            pixel = getPixel(x, y, buf, w, h, type);
            setPixel(pixel, xx, yy, qpixelBuf, w2, h2, type);
            setPixel(pixel, xx+1, yy, qpixelBuf, w2, h2, type);
            setPixel(pixel, xx, yy+1, qpixelBuf, w2, h2, type);
            setPixel(pixel, xx+1, yy+1, qpixelBuf, w2, h2, type);
            xx+=2;
        }
        yy+=2;
    }
}


/**
 * Converts an image buffer back from a qpixel-representation to normal, i.e.
 * shrinks the whole image both horizontally and vertically by factor 2
 * (leading to a factor 4 decrease in total).
 * buf must have been allocated before with 1/4-times amount of memory as
 * qpixelBuf.
 */
void convertFromQpixels(char* qpixelBuf, unsigned char* buf, int w, int h, int type) {  //, int qpixelDepth) {
    int x;
    int y;
    int xx;
    int yy;
    int pixel;
    int a,b,c,d;
    int w2;
    int h2;
    
    w2 = w*2;
    h2 = h*2;
    yy = 0;
    for (y=0; y<h; y++) {
        xx = 0;
        for (x=0; x<w; x++) {
            a = getPixel(xx, yy, qpixelBuf, w2, h2, type);
            b = getPixel(xx+1, yy, qpixelBuf, w2, h2, type);
            c = getPixel(xx, yy+1, qpixelBuf, w2, h2, type);
            d = getPixel(xx+1, yy+1, qpixelBuf, w2, h2, type);
            pixel = (a+b+c+d)/4;
            setPixel(pixel, x, y, buf, w, h, type);
            xx+=2;
        }
        yy+=2;
    }
}


/* --- mask-detection ----------------------------------------------------- */

/**
 * Finds one edge of non-black pixels headig from one starting point towards edge direction.
 * @return number of shift-steps until blank edge found
 */
int detectEdge(int startX, int startY, int shiftX, int shiftY, int maskScanSize, int maskScanDepth, float maskScanThreshold, unsigned char* buf, int w, int h, int type) {
    // either shiftX or shiftY is 0, the other value is -i|+i
    int left;
    int top;
    int right;
    int bottom;
    int half;
    int halfDepth;
    int blackness;
    int total;
    int count;
    
    half = maskScanSize / 2;
    total = 0;
    count = 0;
    if (shiftY==0) { // vertical border is to be detected, horizontal shifting of scan-bar
        if (maskScanDepth == -1) {
            maskScanDepth = h;
        }
        halfDepth = maskScanDepth / 2;
        left = startX - half;
        top = startY - halfDepth;
        right = startX + half;
        bottom = startY + halfDepth;
    } else { // horizontal border is to be detected, vertical shifting of scan-bar
        if (maskScanDepth == -1) {
            maskScanDepth = w;
        }
        halfDepth = maskScanDepth / 2;
        left = startX - halfDepth;
        top = startY - half;
        right = startX + halfDepth;
        bottom = startY + half;
    }
    
    while (1==1) { // !
        blackness = 255 - brightnessRect(left, top, right, bottom, buf, w, h, type);
        total += blackness;
        count++;
        // is blackness below treshold*average?
        if ((blackness < ((maskScanThreshold*total)/count))||(blackness==0)) { // this will surely become true when pos reaches the outside of the actual image area and blacknessRect() will deliver 0 because all pixels outside are considered white
            return count; // return here - always return absolute value of shifting difference //pos;
        }
        left += shiftX;
        right += shiftX;
        top += shiftY;
        bottom += shiftY;
    }
}


/**
 * Detects a mask of white borders around a starting point.
 * The result is returned via call-by-reference parameters left, top, right, bottom.
 * @return the detected mask in left, top, right, bottom; or -1, -1, -1, -1 if no mask could be detected
 */
BOOLEAN detectMask(int startX, int startY, int maskScanDirections, int maskScanSize[DIRECTIONS_COUNT], int maskScanDepth[DIRECTIONS_COUNT], int maskScanStep[DIRECTIONS_COUNT], float maskScanThreshold[DIRECTIONS_COUNT], int maskScanMinimum[DIMENSIONS_COUNT], int maskScanMaximum[DIMENSIONS_COUNT], int* left, int* top, int* right, int* bottom, unsigned char* buf, int w, int h, int type) {
    int width;
    int height;
    int half[DIRECTIONS_COUNT];
    BOOLEAN success;
    
    half[HORIZONTAL] = maskScanSize[HORIZONTAL] / 2;
    half[VERTICAL] = maskScanSize[VERTICAL] / 2;
    if ((maskScanDirections & 1<<HORIZONTAL) != 0) {
        *left = startX - maskScanStep[HORIZONTAL] * detectEdge(startX, startY, -maskScanStep[HORIZONTAL], 0, maskScanSize[HORIZONTAL], maskScanDepth[HORIZONTAL], maskScanThreshold[HORIZONTAL], buf, w, h, type) - half[HORIZONTAL];
        *right = startX + maskScanStep[HORIZONTAL] * detectEdge(startX, startY, maskScanStep[HORIZONTAL], 0, maskScanSize[HORIZONTAL], maskScanDepth[HORIZONTAL], maskScanThreshold[HORIZONTAL], buf, w, h, type) + half[HORIZONTAL];
    } else { // full range of sheet
        *left = 0;
        *right = w - 1;
    }
    if ((maskScanDirections & 1<<VERTICAL) != 0) {
        *top = startY - maskScanStep[VERTICAL] * detectEdge(startX, startY, 0, -maskScanStep[VERTICAL], maskScanSize[VERTICAL], maskScanDepth[VERTICAL], maskScanThreshold[VERTICAL], buf, w, h, type) - half[VERTICAL];
        *bottom = startY + maskScanStep[VERTICAL] * detectEdge(startX, startY, 0, maskScanStep[VERTICAL], maskScanSize[VERTICAL], maskScanDepth[VERTICAL], maskScanThreshold[VERTICAL], buf, w, h, type) + half[VERTICAL];
    } else { // full range of sheet
        *top = 0;
        *bottom = h	 - 1;
    }
    
    // if below minimum or above maximum, set to maximum
    width = *right - *left;
    height = *bottom - *top;
    success = TRUE;
    if (width < maskScanMinimum[WIDTH] || width > maskScanMaximum[WIDTH]) {
        width = maskScanMaximum[WIDTH] / 2;
        *left = startX - width;
        *right = startX + width;
        success = FALSE;;
    }
    if (height < maskScanMinimum[HEIGHT] || height > maskScanMaximum[HEIGHT]) {
        height = maskScanMaximum[HEIGHT] / 2;
        *top = startY - height;
        *bottom = startY + height;
        success = FALSE;
    }
    return success;
}


/**
 * Detects masks around the points specified in point[].
 * @param mask point to array into which detected masks will be stored
 * @return number of masks stored in mask[][]
 */
int detectMasks(int mask[MAX_MASKS][EDGES_COUNT], BOOLEAN maskValid[MAX_MASKS], int point[MAX_POINTS][COORDINATES_COUNT], int pointCount, int maskScanDirections, int maskScanSize[DIRECTIONS_COUNT], int maskScanDepth[DIRECTIONS_COUNT], int maskScanStep[DIRECTIONS_COUNT], float maskScanThreshold[DIRECTIONS_COUNT], int maskScanMinimum[DIMENSIONS_COUNT], int maskScanMaximum[DIMENSIONS_COUNT],  unsigned char* buf, int w, int h, int type) {
    int left;
    int top;
    int right;
    int bottom;
    int i;
    int maskCount;
    
    maskCount = 0;
    if (maskScanDirections != 0) {
         for (i = 0; i < pointCount; i++) {
             maskValid[i] = detectMask(point[i][X], point[i][Y], maskScanDirections, maskScanSize, maskScanDepth, maskScanStep, maskScanThreshold, maskScanMinimum, maskScanMaximum, &left, &top, &right, &bottom, buf, w, h, type);
             if (!(left==-1 || top==-1 || right==-1 || bottom==-1)) {
                 mask[maskCount][LEFT] = left;
                 mask[maskCount][TOP] = top;
                 mask[maskCount][RIGHT] = right;
                 mask[maskCount][BOTTOM] = bottom;
                 maskCount++;
                 if (verbose>=VERBOSE_NORMAL) {
                     printf("auto-masking (%i,%i): %i,%i,%i,%i", point[i][X], point[i][Y], left, top, right, bottom);
                     if (maskValid[i] == FALSE) { // (mask had been auto-set to full page size)
                         printf(" (invalid detection, using full page size)");
                     }
                     printf("\n");
                 }
             } else {
                 if (verbose>=VERBOSE_NORMAL) {
                     printf("auto-masking (%i,%i): NO MASK FOUND\n", point[i][X], point[i][Y]);
                 }
             }
             //if (maskValid[i] == FALSE) { // (mask had been auto-set to full page size)
             //    if (verbose>=VERBOSE_NORMAL) {
             //        printf("auto-masking (%i,%i): NO MASK DETECTED\n", point[i][X], point[i][Y]);
             //    }
             //}
         }
    }
    return maskCount;
}


/**
 * Permanently applies image masks. Each pixel which is not covered by at least
 * one mask is set to maskColor.
 */
void applyMasks(int mask[MAX_MASKS][EDGES_COUNT], int maskCount, int maskColor, unsigned char* buf, int w, int h, int type) {
    int x;
    int y;
    int pixel;
    int total;
    int count;
    int i;
    int left, top, right, bottom;
    BOOLEAN m;
    
    if (maskCount<=0) {
        return;
    }
    for (y=0; y<h; y++) {
        for (x=0; x<w; x++) {
            // in any mask?
            m = FALSE;
            for (i=0; ((m==FALSE) && (i<maskCount)); i++) {
                left = mask[i][LEFT];
                top = mask[i][TOP];
                right = mask[i][RIGHT];
                bottom = mask[i][BOTTOM];
                if (y>=top && y<=bottom && x>=left && x<=right) {
                    m = TRUE;
                }
            }
            if (m == FALSE) {
                setPixel(maskColor, x, y, buf, w, h, type); // delete: set to white
            }
        }
    }
}


/* --- wiping ------------------------------------------------------------- */

/**
 * Permanently wipes out areas of an images. Each pixel covered by a wipe-area
 * is set to wipeColor.
 */
void applyWipes(int area[MAX_MASKS][EDGES_COUNT], int areaCount, int wipeColor, unsigned char* buf, int w, int h, int type) {
    int x;
    int y;
    int i;
    int count;
    int pixel;

    for (i = 0; i < areaCount; i++) {
        count = 0;
        for (y = area[i][TOP]; y <= area[i][BOTTOM]; y++) {
            for (x = area[i][LEFT]; x <= area[i][RIGHT]; x++) {
                if ( setPixel(wipeColor, x, y, buf, w, h, type) ) {
                    count++;
                }
            }
        }
        if (verbose >= VERBOSE_MORE) {
            printf("wipe [%i,%i,%i,%i]: %i pixels\n", area[i][LEFT], area[i][TOP], area[i][RIGHT], area[i][BOTTOM], count);
        }
    }
}


/* --- mirroring ---------------------------------------------------------- */

/**
 * Mirrors an image either horizontally, vertically, or both.
 */
void mirror(int directions, unsigned char* buf, int w, int h, int type) {
    int x;
    int y;
    int xx;
    int yy;
    int pixel1;
    int pixel2;
    BOOLEAN horizontal;
    BOOLEAN vertical;
    int untilX;
    int untilY;
    
    horizontal = ((directions & 1<<HORIZONTAL) != 0) ? TRUE : FALSE;
    vertical = ((directions & 1<<VERTICAL) != 0) ? TRUE : FALSE;
    untilX = ((horizontal==TRUE)&&(vertical==FALSE)) ? ((w-1)>>1) : w-1; // w>>1 == (int)(w-0.5)/2
    untilY = (vertical==TRUE) ? ((h-1)>>1) : h-1;
    for (y = 0; y <= untilY; y++) {
        for (x = 0; x <= untilX; x++) {
            xx = (horizontal==TRUE) ? w - x - 1 : x;
            yy = (vertical==TRUE) ? h - y - 1 : y;
            pixel1 = getPixel(x, y, buf, w, h, type);
            pixel2 = getPixel(xx, yy, buf, w, h, type);
            setPixel(pixel2, x, y, buf, w, h, type);
            setPixel(pixel1, xx, yy, buf, w, h, type);
        }
    }
}


/* --- flip-rotating ------------------------------------------------------ */

/**
 * Rotates an image clockwise or anti-clockwise in 90-degrees steps.
 * @param direction either -1 (rotate anti-clockwise) or 1 (rotate clockwise)
 */
void flipRotate(int direction, unsigned char** buf, int* width, int* height, int type) {
    unsigned char* buf2;
    int bufSize;
    int x;
    int y;
    int xx;
    int yy;
    int pixel;
    int tmp;
    
    bufSize = (*width) * (*height);
    buf2 = (unsigned char*)malloc(bufSize);
    for (y = 0; y < *height; y++) {
        xx = ((direction > 0)?*height-1:0) - y*direction;
        for (x = 0; x < *width; x++) {
            yy = ((direction < 0)?*width-1:0) + x*direction;
            pixel = getPixel(x, y, *buf, *width, *height, type);
            setPixel(pixel, xx, yy, buf2, *height, *width, type);
        }
    }
    free(*buf);
    *buf = buf2;
    tmp = *height;
    *height = *width;
    *width = tmp;
}


/* --- blackfilter -------------------------------------------------------- */

/**
 * Filters out solidly black areas scanning to one direction.
 * @param stepX is 0 if stepY!=0
 * @param stepY is 0 if stepX!=0
 * @see blackfilter()
 */
void blackfilterScan(int stepX, int stepY, int size, int depth, int step, float threshold, int intensity, float blackThreshold, unsigned char* buf, int w, int h, int type) {
    int left;
    int top;
    int right;
    int bottom;
    int blackness;
    int thresholdBlack;
    int x;
    int y;
    int shiftX;
    int shiftY;
    int l, t, r, b;
    int total;
    int thresholdAbs;
    int diffX;
    int diffY;

    thresholdBlack = (int)(WHITE * (1.0-blackThreshold));
    total = size*depth;
    thresholdAbs = (int)(total * threshold);
    if (stepX != 0) { // horizontal scanning
        left = 0;
        top = 0;
        right = size -1;
        bottom = depth - 1;
        shiftX = 0;
        shiftY = depth;
    } else { // vertical scanning
        left = 0;
        top = 0;
        right = depth -1;
        bottom = size - 1;
        shiftX = depth;
        shiftY = 0;
    }
    while ((left < w) && (top < h)) { // individual scanning "stripes" over the whole sheet
        l = left;
        t = top;
        r = right;
        b = bottom;
        // make sure last stripe does not reach outside sheet, shift back inside (next +=shift will exit while-loop)
        if (r>=w || b>=h) {
            diffX = r-w+1;
            diffY = b-h+1;
            l -= diffX;
            t -= diffY;
            r -= diffX;
            b -= diffY;
        }
        while ((l < w) && (t < h)) { // single scanning "stripe"
            blackness = countPixelsRect(l, t, r, b, 0, thresholdBlack, FALSE, buf, w, h, type);
            if (blackness >= thresholdAbs) { // found a solidly black area
                if (verbose >= VERBOSE_NORMAL) {
                    printf("black-area flood-fill: [%i,%i,%i,%i]\n", l, t, r, b);
                }
                // start flood-fill in this area (on each pixel to make sure we get everything, in most cases first flood-fill from first pixel will delete all other black pixels in the area already)
                for (y = t; y <= b; y++) {
                    for (x = l; x <= r; x++) {
                        floodFill(x, y, WHITE, 0, thresholdBlack, intensity, buf, w, h, type);
                    }
                }
            }
            l += stepX;
            t += stepY;
            r += stepX;
            b += stepY;
        }
        left += shiftX;
        top += shiftY;
        right += shiftX;
        bottom += shiftY;
    }
}


/**
 * Filters out solidly black areas, as appearing on bad photocopies.
 * A virtual bar of width 'size' and height 'depth' is horizontally moved 
 * above the middle of the sheet (or the full sheet, if depth ==-1).
 */
void blackfilter(int blackfilterScanDirections, int blackfilterScanSize[DIRECTIONS_COUNT], int blackfilterScanDepth[DIRECTIONS_COUNT], int blackfilterScanStep[DIRECTIONS_COUNT], float blackfilterScanThreshold, int blackfilterIntensity, float blackThreshold, unsigned char* buf, int w, int h, int type) {
    if ((blackfilterScanDirections & 1<<HORIZONTAL) != 0) { // left-to-right scan
        blackfilterScan(blackfilterScanStep[HORIZONTAL], 0, blackfilterScanSize[HORIZONTAL], blackfilterScanDepth[HORIZONTAL], blackfilterScanStep[HORIZONTAL], blackfilterScanThreshold, blackfilterIntensity, blackThreshold, buf, w, h, type);
    }
    if ((blackfilterScanDirections & 1<<VERTICAL) != 0) { // top-to-bottom scan
        blackfilterScan(0, blackfilterScanStep[VERTICAL], blackfilterScanSize[VERTICAL], blackfilterScanDepth[VERTICAL], blackfilterScanStep[VERTICAL], blackfilterScanThreshold, blackfilterIntensity, blackThreshold, buf, w, h, type);
    }
}


/* --- noisefilter -------------------------------------------------------- */

/**
 * Applies a simple noise filter to the image.
 * @param intensity maximum cluster size to delete
 */
int noisefilter(int intensity, float whiteThreshold, unsigned char* buf, int w, int h, int type) {
    int x;
    int y;
    int whiteMin;
    int count;
    int pixel;
    int neighbors;
    
    whiteMin = (int)(WHITE * whiteThreshold);
    count = 0;
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            pixel = getPixel(x, y, buf, w, h, type);
            if (pixel < whiteMin) {
                neighbors = countPixelNeighbors(x, y, intensity, whiteMin, buf, w, h, type);
                if (neighbors <= intensity) {
                    clearPixelNeighbors(x, y, whiteMin, buf, w, h, type);
                    count++;
                }
            }
        }
    }
    return count;
}


/* --- blurfilter --------------------------------------------------------- */

/**
 * Removes noise using a kind of blurfilter, as alternative to the noise
 * filter. This algoithm counts pixels while 'shaking' the area to detect,
 * and clears the area if the amount of white pixels exceeds whiteTreshold.
 */
int blurfilter(int blurfilterScanSize[DIRECTIONS_COUNT], int blurfilterScanStep[DIRECTIONS_COUNT], float blurfilterIntensity, float whiteThreshold, unsigned char* buf, int w, int h, int type) {
    int whiteMin;
    int left;
    int top;
    int right;
    int bottom;
    int count;
    int max;
    int total;
    int result;
    
    result = 0;
    whiteMin = (int)(WHITE * whiteThreshold);
    left = 0;
    top = 0;
    right = blurfilterScanSize[HORIZONTAL] - 1;
    bottom = blurfilterScanSize[VERTICAL] - 1;
    total = blurfilterScanSize[HORIZONTAL] * blurfilterScanSize[VERTICAL];
    
    while (TRUE) { // !
        max = 0;
        count = countPixelsRect(left, top, right, bottom, 0, whiteMin, FALSE, buf, w, h, type);
        if (count > max) {
            max = count;
        }
        count = countPixelsRect(left-blurfilterScanStep[HORIZONTAL], top-blurfilterScanStep[VERTICAL], right-blurfilterScanStep[HORIZONTAL], bottom-blurfilterScanStep[VERTICAL], 0, whiteMin, FALSE, buf, w, h, type);
        if (count > max) {
            max = count;
        }
        count = countPixelsRect(left+blurfilterScanStep[HORIZONTAL], top-blurfilterScanStep[VERTICAL], right+blurfilterScanStep[HORIZONTAL], bottom-blurfilterScanStep[VERTICAL], 0, whiteMin, FALSE, buf, w, h, type);
        if (count > max) {
            max = count;
        }
        count = countPixelsRect(left-blurfilterScanStep[HORIZONTAL], top+blurfilterScanStep[VERTICAL], right-blurfilterScanStep[HORIZONTAL], bottom+blurfilterScanStep[VERTICAL], 0, whiteMin, FALSE, buf, w, h, type);
        if (count > max) {
            max = count;
        }
        count = countPixelsRect(left+blurfilterScanStep[HORIZONTAL], top+blurfilterScanStep[VERTICAL], right+blurfilterScanStep[HORIZONTAL], bottom+blurfilterScanStep[VERTICAL], 0, whiteMin, FALSE, buf, w, h, type);
        if (count > max) {
            max = count;
        }
        if ((((float)max)/total) <= blurfilterIntensity) {
            result += countPixelsRect(left, top, right, bottom, 0, whiteMin, TRUE, buf, w, h, type); // also clear
        }
        if (right < w) { // not yet at end of row
            left += blurfilterScanStep[HORIZONTAL];
            right += blurfilterScanStep[HORIZONTAL];
        } else { // end of row
            if (bottom >= h) { // has been last row
                return result; // exit here
            }
            // next row:
            left = 0;
            right = blurfilterScanSize[HORIZONTAL] - 1;
            top += blurfilterScanStep[VERTICAL];
            bottom += blurfilterScanStep[VERTICAL];
        }
    }
}


/* --- grayfilter --------------------------------------------------------- */

/**
 * Clears areas which do not contain any black pixels, but some "gray shade" only.
 * Two conditions have to apply before an area gets deleted: first, not a single black pixel may be contained,
 * second, a minimum threshold of blackness must not be exceeded.
 */
int grayfilter(int grayfilterScanSize[DIRECTIONS_COUNT], int grayfilterScanStep[DIRECTIONS_COUNT], float grayfilterThreshold, float blackThreshold, unsigned char* buf, int w, int h, int type) {
    int blackMax;
    int left;
    int top;
    int right;
    int bottom;
    int count;
    int blackness;
    int thresholdAbs;
    int total;
    int result;
    
    result = 0;
    blackMax = (int)(WHITE * (1.0-blackThreshold));
    thresholdAbs = (int)(WHITE * grayfilterThreshold);
    left = 0;
    top = 0;
    right = grayfilterScanSize[HORIZONTAL] - 1;
    bottom = grayfilterScanSize[VERTICAL] - 1;
    total = grayfilterScanSize[HORIZONTAL] * grayfilterScanSize[VERTICAL];
    
    while (TRUE) { // !
        count = countPixelsRect(left, top, right, bottom, 0, blackMax, FALSE, buf, w, h, type);
        if (count == 0) {
            blackness = WHITE - brightnessRect(left, top, right, bottom, buf, w, h, type);
            if (blackness < thresholdAbs) {
                result += fillRect(WHITE, left, top, right, bottom, buf, w, h, type);
            }
        }
        if (left < w) { // not yet at end of row
            left += grayfilterScanStep[HORIZONTAL];
            right += grayfilterScanStep[HORIZONTAL];
        } else { // end of row
            if (bottom >= h) { // has been last row
                return result; // exit here
            }
            // next row:
            left = 0;
            right = grayfilterScanSize[HORIZONTAL] - 1;
            top += grayfilterScanStep[VERTICAL];
            bottom += grayfilterScanStep[VERTICAL];
        }
    }
}


/* --- border-detection --------------------------------------------------- */

/**
 * Moves a rectangular area of pixels to be centered above the centerX, centerY coordinates.
 */
void centerMask(int centerX, int centerY, int left, int top, int right, int bottom, unsigned char* buf, int w, int h, int type) {
    int width;
    int height;
    int targetX;
    int targetY;
    int shiftX;
    int shiftY;
    int x;
    int y;
    int pixel;
    unsigned char* b;
    
    width = right - left + 1;
    height = bottom - top + 1;
    targetX = centerX - width/2;
    targetY = centerY - height/2; 
    if ((targetX >= 0) && (targetY >= 0) && ((targetX+width) <= w) && ((targetY+height) <= h)) {
        if (verbose >= VERBOSE_NORMAL) {
            printf("centering mask (%i,%i): %i, %i\n", centerX, centerY, targetX-left, targetY-top);
        }
        b = (unsigned char*)malloc(width * height);
        copyBuffer(left, top, width, height, buf, w, h, type, 0, 0, b, width, height, type);
        fillRect(WHITE, left, top, right, bottom, buf, w, h, type);
        copyBuffer(0, 0, width, height, b, width, height, type, targetX, targetY, buf, w, h, type);
        free(b);
    } else {
        if (verbose >= VERBOSE_NORMAL) {
            printf("centering mask (%i,%i): %i, %i - NO CENTERING (would shift area outside visible image)\n", centerX, centerY, targetX-left, targetY-top);
        }
    }
}


/**
 * Find the size of one border edge.
 */
int detectBorderEdge(int stepX, int stepY, int size, int threshold, int maxBlack, unsigned char* buf, int w, int h, int type) {
    int left;
    int top;
    int right;
    int bottom;
    int max;
    int cnt;
    int result;
    
    if (stepY == 0) { // horizontal detection
        if (stepX > 0) {
            left = 0;
            top = 0;
            right = size - 1;
            bottom = h - 1;
        } else {
            right = w - 1;
            left = right - size - 1;
            top = 0;
            bottom = h - 1;
        }
        max = w / 2;
    } else { // vertical detection
        if (stepY > 0) {
            left = 0;
            top = 0;
            right = w - 1;
            bottom = size - 1;
        } else {
            left = 0;
            right = w - 1;
            bottom = h - 1;
            top = bottom - size - 1;
        }
        max = h / 2;
    }
    result = 0;
    while (result < max) {
        cnt = countPixelsRect(left, top, right, bottom, 0, maxBlack, FALSE, buf, w, h, type);
        if (cnt >= threshold) {
            return result; // border has been found: regular exit here
        }
        left += stepX;
        top += stepY;
        right += stepX;
        bottom += stepY;
        result += abs(stepX+stepY); // (either stepX or stepY is 0)
    }
    return 0; // no border found between 0..max
}


/**
 * Detects a border of completely non-black pixels around a whole sheet.
 */
void detectBorder(int border[EDGES_COUNT], int borderScanDirections, int borderScanSize[DIRECTIONS_COUNT], int borderScanStep[DIRECTIONS_COUNT], int borderScanThreshold[DIRECTIONS_COUNT], float blackThreshold, unsigned char* buf, int w, int h, int type) {
    int blackThresholdAbs;
    
    blackThresholdAbs = (int)(WHITE * (1.0-blackThreshold));
    if (borderScanDirections & 1<<HORIZONTAL) {
        border[LEFT] = detectBorderEdge(borderScanStep[HORIZONTAL], 0, borderScanSize[HORIZONTAL], borderScanThreshold[HORIZONTAL], blackThresholdAbs, buf, w, h, type);
        border[RIGHT] = detectBorderEdge(-borderScanStep[HORIZONTAL], 0, borderScanSize[HORIZONTAL], borderScanThreshold[HORIZONTAL], blackThresholdAbs, buf, w, h, type);
    }
    if (borderScanDirections & 1<<VERTICAL) {
        border[TOP] = detectBorderEdge(0, borderScanStep[VERTICAL], borderScanSize[VERTICAL], borderScanThreshold[VERTICAL], blackThresholdAbs, buf, w, h, type);
        border[BOTTOM] = detectBorderEdge(0, -borderScanStep[VERTICAL], borderScanSize[VERTICAL], borderScanThreshold[VERTICAL], blackThresholdAbs, buf, w, h, type);
    }
    if (verbose >= VERBOSE_NORMAL) {
        printf("border detected: (%i,%i,%i,%i)\n", border[LEFT], border[TOP], border[RIGHT], border[BOTTOM]);
    }
}


/**
 * Converts a border-tuple to a mask-tuple.
 */
void borderToMask(int border[EDGES_COUNT], int mask[EDGES_COUNT], int w, int h) {
    mask[LEFT] = border[LEFT];
    mask[TOP] = border[TOP];
    mask[RIGHT] = w - border[RIGHT] - 1;
    mask[BOTTOM] = h - border[BOTTOM] - 1;
}


/**
 * Applies a border to the whole image. All pixels in the border range at the
 * edges of the sheet will be cleared.
 */
void applyBorder(int border[EDGES_COUNT], int borderColor, unsigned char* buf, int w, int h, int type) {
    int mask[EDGES_COUNT];

    if (border[LEFT]!=0 || border[TOP]!=0 || border[RIGHT]!=0 || border[BOTTOM]!=0) {
        borderToMask(border, mask, w, h);
        if (verbose >= VERBOSE_NORMAL) {
            printf("applying border (%i,%i,%i,%i) [%i,%i,%i,%i]\n", border[LEFT], border[TOP], border[RIGHT], border[BOTTOM], mask[LEFT], mask[TOP], mask[RIGHT], mask[BOTTOM]);
        }
        applyMasks(&mask, 1, borderColor, buf, w, h, type);
    }
}


/**
 * Shifts the area inside a border to be centered on the sheet.
 */
void centerBorder(int border[EDGES_COUNT], unsigned char* buf, int w, int h, int type) {
    int mask[EDGES_COUNT];

    borderToMask(border, mask, w, h);
    if (verbose >= VERBOSE_NORMAL) {
        printf("centering border, ");
    }
    centerMask(w/2, h/2, mask[LEFT], mask[TOP], mask[RIGHT], mask[BOTTOM], buf, w, h, type);
}



/****************************************************************************
 * MAIN()                                                                   *
 ****************************************************************************/

/**
 * The main program.
 */
int main(int argc, char* argv[])
{
    // --- parameter variables ---
    int layout;
    int startSheet;
    int endSheet;
    int preRotate;
    int postRotate;
    int preMirror;
    int postMirror;
    int pages;
    int pointCount;
    int point[MAX_POINTS][COORDINATES_COUNT];
    int maskCount;
    int mask[MAX_MASKS][EDGES_COUNT];
    int wipeCount;
    int wipe[MAX_MASKS][EDGES_COUNT];
    int middleWipe[2];
    int preWipeCount;
    int preWipe[MAX_MASKS][EDGES_COUNT];
    int postWipeCount;
    int postWipe[MAX_MASKS][EDGES_COUNT];
    int preBorder[EDGES_COUNT];
    int postBorder[EDGES_COUNT];
    int border[EDGES_COUNT];
    BOOLEAN maskValid[MAX_MASKS];
    int preMaskCount;
    int preMask[MAX_MASKS][EDGES_COUNT];
    int blackfilterScanDirections;
    int blackfilterScanSize[DIRECTIONS_COUNT];
    int blackfilterScanDepth[DIRECTIONS_COUNT];
    int blackfilterScanStep[DIRECTIONS_COUNT];
    float blackfilterScanThreshold;    
    int blackfilterIntensity;
    int noisefilterIntensity;
    int blurfilterScanSize[DIRECTIONS_COUNT];
    int blurfilterScanStep[DIRECTIONS_COUNT];
    float blurfilterIntensity;
    int grayfilterScanSize[DIRECTIONS_COUNT];
    int grayfilterScanStep[DIRECTIONS_COUNT];
    float grayfilterThreshold;
    int maskScanDirections;
    int maskScanSize[DIRECTIONS_COUNT];
    int maskScanDepth[DIRECTIONS_COUNT];
    int maskScanStep[DIRECTIONS_COUNT];
    float maskScanThreshold[DIRECTIONS_COUNT];
    int maskScanMinimum[DIMENSIONS_COUNT];
    int maskScanMaximum[DIMENSIONS_COUNT];
    int maskColor;
    int deskewScanDirections;
    int deskewScanSize;
    float deskewScanDepth;
    float deskewScanRange;
    float deskewScanStep;
    float deskewScanThreshold;
    float deskewScanDeviation;
    int borderScanDirections;
    int borderScanSize[DIRECTIONS_COUNT];
    int borderScanStep[DIRECTIONS_COUNT];
    int borderScanThreshold[DIRECTIONS_COUNT];
    float whiteThreshold;
    float blackThreshold;
    BOOLEAN writeoutput;
    BOOLEAN qpixels;
    BOOLEAN multisheets;
    char* outputTypeName; 
    int noBlackfilterMultiIndex[MAX_MULTI_INDEX];
    int noBlackfilterMultiIndexCount;
    int noNoisefilterMultiIndex[MAX_MULTI_INDEX];
    int noNoisefilterMultiIndexCount;
    int noBlurfilterMultiIndex[MAX_MULTI_INDEX];
    int noBlurfilterMultiIndexCount;
    int noGrayfilterMultiIndex[MAX_MULTI_INDEX];
    int noGrayfilterMultiIndexCount;
    int noMaskScanMultiIndex[MAX_MULTI_INDEX];
    int noMaskScanMultiIndexCount;
    int noMaskCenterMultiIndex[MAX_MULTI_INDEX];
    int noMaskCenterMultiIndexCount;
    int noDeskewMultiIndex[MAX_MULTI_INDEX];
    int noDeskewMultiIndexCount;
    int noWipeMultiIndex[MAX_MULTI_INDEX];
    int noWipeMultiIndexCount;
    int noBorderMultiIndex[MAX_MULTI_INDEX];
    int noBorderMultiIndexCount;
    int noBorderScanMultiIndex[MAX_MULTI_INDEX];
    int noBorderScanMultiIndexCount;
    int noBorderCenterMultiIndex[MAX_MULTI_INDEX];
    int noBorderCenterMultiIndexCount;
    int sheetMultiIndex[MAX_MULTI_INDEX];
    int sheetMultiIndexCount;    
    int excludeMultiIndex[MAX_MULTI_INDEX];
    int excludeMultiIndexCount;
    int ignoreMultiIndex[MAX_MULTI_INDEX];
    int ignoreMultiIndexCount;    
    // --- local variables ---
    int x;
    int y;
    int width;
    int height;
    int left;
    int top;
    int right;
    int bottom;
    int i;
    int j;
    char* inputFilename; 
    char* outputFilename; 
    char inputFilenameResolved[255];
    char outputFilenameResolved[255];
    int autoborder[EDGES_COUNT];
    int fileSize;
    int inputSize;
    int inputSizeFile;
    unsigned char* buffer; // binary image data
    unsigned char* originalBuffer; // binary image data
    unsigned char* qpixelBuffer; // binary image data
    char* layoutStr;
    char* inputTypeName; 
    int inputType;
    int filterResult;
    double rotation;
    int q;
    int rWidth;
    int rHeight;
    int rSize;
    unsigned char *r; // binary image data
    unsigned char *rSource; // binary image data
    int borderMask[EDGES_COUNT];
    int outputType;
    BOOLEAN success;
    int nr;
    clock_t startTime;
    clock_t endTime;
    clock_t time;
    clock_t totalTime;
    int totalCount;
    int exitCode;

    exitCode = 0; // error code to return

    
    // --- process all sheets ------------------------------------------------
    
    // count from start sheet to end sheet
    startSheet = 1; // defaults, may be changed in first run of for-loop
    endSheet = -1;
    totalTime = 0;
    totalCount = 0;
    for (nr = startSheet; (endSheet == -1) || (nr <= endSheet); nr++) {

        // --- default parameter values ---
        layout = -1;
        layoutStr = "NONE";
        preRotate = 0;
        postRotate = 0;
        preMirror = 0;
        postMirror = 0;
        outputTypeName = NULL; // set default later
        pages = 0;             // set default later
        pointCount = 0;
        maskCount = 0;
        preMaskCount = 0;
        wipeCount = 0;
        preWipeCount = 0;
        postWipeCount = 0;
        middleWipe[0] = middleWipe[1] = 0; // left/right
        border[LEFT] = border[TOP] = border[RIGHT] = border[BOTTOM] = 0;
        preBorder[LEFT] = preBorder[TOP] = preBorder[RIGHT] = preBorder[BOTTOM] = 0;
        postBorder[LEFT] = postBorder[TOP] = postBorder[RIGHT] = postBorder[BOTTOM] = 0;
        blackfilterScanDirections = (1<<HORIZONTAL) | (1<<VERTICAL);
        blackfilterScanSize[HORIZONTAL] = blackfilterScanSize[VERTICAL] = 20;
        blackfilterScanDepth[HORIZONTAL] = blackfilterScanDepth[VERTICAL] = 500;
        blackfilterScanStep[HORIZONTAL] = blackfilterScanStep[VERTICAL] = 5;
        blackfilterScanThreshold = 0.95;
        blackfilterIntensity = 20;
        noisefilterIntensity = 4;
        blurfilterScanSize[HORIZONTAL] = blurfilterScanSize[VERTICAL] = 100;
        blurfilterScanStep[HORIZONTAL] = blurfilterScanStep[VERTICAL] = 50;
        blurfilterIntensity = 0.01;
        grayfilterScanSize[HORIZONTAL] = grayfilterScanSize[VERTICAL] = 50;
        grayfilterScanStep[HORIZONTAL] = grayfilterScanStep[VERTICAL] = 20;
        grayfilterThreshold = 0.5;
        maskScanDirections = (1<<HORIZONTAL);
        maskScanSize[HORIZONTAL] = maskScanSize[VERTICAL] = 50;
        maskScanDepth[HORIZONTAL] = maskScanDepth[VERTICAL] = -1;
        maskScanStep[HORIZONTAL] = maskScanStep[VERTICAL] = 10;
        maskScanThreshold[HORIZONTAL] = maskScanThreshold[VERTICAL] = 0.1;
        maskScanMinimum[WIDTH] = maskScanMinimum[HEIGHT] = 100;
        maskScanMaximum[WIDTH] = maskScanMaximum[HEIGHT] = -1; // set default later
        maskColor = 255;
        deskewScanDirections = (1<<HORIZONTAL);
        deskewScanSize = 1500;
        deskewScanDepth = 0.666666;
        deskewScanRange = 2.0;
        deskewScanStep = 0.1;
        deskewScanThreshold = 0.1; // (unused)
        deskewScanDeviation = 1.0;
        borderScanDirections = (1<<VERTICAL);
        borderScanSize[HORIZONTAL] = borderScanSize[VERTICAL] = 5;
        borderScanStep[HORIZONTAL] = borderScanStep[VERTICAL] = 5;
        borderScanThreshold[HORIZONTAL] = borderScanThreshold[VERTICAL] = 5;
        whiteThreshold = 0.9;
        blackThreshold = 0.5;
        writeoutput = TRUE;
        qpixels = TRUE;
        multisheets = TRUE;
        verbose = VERBOSE_NONE;
        noBlackfilterMultiIndexCount = 0; // 0: allow all, -1: disable all, n: individual entries
        noNoisefilterMultiIndexCount = 0;
        noBlurfilterMultiIndexCount = 0;
        noGrayfilterMultiIndexCount = 0;
        noMaskScanMultiIndexCount = 0;
        noMaskCenterMultiIndexCount = 0;
        noDeskewMultiIndexCount = 0;
        noWipeMultiIndexCount = 0;
        noBorderMultiIndexCount = 0;
        noBorderScanMultiIndexCount = 0;
        noBorderCenterMultiIndexCount = 0;
        sheetMultiIndexCount = -1; // default: process all between start-sheet and end-sheet
        excludeMultiIndexCount = 0;
        ignoreMultiIndexCount = 0;


        // --- parse parameters ----------------------------------------------
        i = 1;
        while ((argc==0) || ((i < argc) && (argv[i][0]=='-'))) {

            // --help
            if (argc==0 || strcmp(argv[i], "--help")==0 || strcmp(argv[i], "-h")==0 || strcmp(argv[i], "-?")==0 || strcmp(argv[i], "/?")==0 || strcmp(argv[i], "?")==0) {
                printf(WELCOME, VERSION);
                printf("\n");
                printf(USAGE);
                printf("Options are:\n");
                printf(OPTIONS);
                return 0;

            // --help-options (undocumented, used by build-process)
            } else if (strcmp(argv[i], "--help-options")==0) {
                printf(OPTIONS);
                return 0;

            // --help-usage (undocumented, used by build-process)
            } else if (strcmp(argv[i], "--help-usage")==0) {
                printf(USAGE);
                return 0;

            // --help-readme (undocumented, used by build-process)
            } else if (strcmp(argv[i], "--help-readme")==0) {
                printf(README);
                return 0;

            // --help-compile (undocumented, used by build-process)
            } else if (strcmp(argv[i], "--help-compile")==0) {
                printf(COMPILE);
                return 0;

            // --version -V
            } else if (strcmp(argv[i], "-V")==0 || strcmp(argv[i], "--version")==0) {
                if (BUILD != NULL) {
                    printf("%s (build %s)\n", VERSION, BUILD);
                } else {
                    printf("%s\n", VERSION);
                }
                return 0;

            // --version-number (undocumented, used by build-process)
            } else if (strcmp(argv[i], "--version-number")==0) {
                printf("%s\n", VERSION);
                return 0;

            // --version-build (undocumented, used by build-process)
            } else if (strcmp(argv[i], "--version-build")==0) {
                if (BUILD != NULL) {
                    printf("%s\n", BUILD);
                }
                return 0;

            // --layout  -l
            } else if (strcmp(argv[i], "-l")==0 || strcmp(argv[i], "--layout")==0) {
                i++;
                noMaskCenterMultiIndexCount = 0; // enable mask centering
                if (strcmp(argv[i], "single")==0) {
                    layout = LAYOUT_SINGLE;
                } else if (strcmp(argv[i], "double")==0) {
                    layout = LAYOUT_DOUBLE;
                    // assume pages are rotated right-above-left on the sheet, so pre-rotate
                    preRotate = 90; // default as set by layout-template here, may again be overwritten by specific option
                    postRotate = -90;
                } else {
                    printf("*** error: Unknown layout mode '%s'.", argv[i]);
                    exitCode = 1;
                }

            // --start-sheet
            } else if ((strcmp(argv[i], "-s")==0)||(strcmp(argv[i], "--start-sheet")==0)) {
                sscanf(argv[++i],"%i", &startSheet);
                if (nr < startSheet) {
                    nr = startSheet;
                }

            // --end-sheet
            } else if ((strcmp(argv[i], "-e")==0)||(strcmp(argv[i], "--end-sheet")==0)) {
                sscanf(argv[++i],"%i", &endSheet);

            // --sheet -#
            } else if ((strcmp(argv[i], "-#")==0)||(strcmp(argv[i], "--sheet")==0)) {
                //sscanf(argv[++i],"%i", &startSheet);
                parseMultiIndex(&i, argv, sheetMultiIndex, &sheetMultiIndexCount);
                if (sheetMultiIndexCount > 0) {
                    if (startSheet < sheetMultiIndex[0]) {
                        startSheet = sheetMultiIndex[0];
                        nr = startSheet;
                    }
                }

            // --exclude  -x
            } else if (strcmp(argv[i], "-x")==0 || strcmp(argv[i], "--exclude")==0) {
                parseMultiIndex(&i, argv, excludeMultiIndex, &excludeMultiIndexCount);
                if (excludeMultiIndexCount == -1) {
                    excludeMultiIndexCount = 0; // 'exclude all' makes no sence
                }

            // --no-processing  -n
            } else if (strcmp(argv[i], "-n")==0 || strcmp(argv[i], "--no-processing")==0) {
                parseMultiIndex(&i, argv, ignoreMultiIndex, &ignoreMultiIndexCount);



            // --pre-rotate
            } else if (strcmp(argv[i], "--pre-rotate")==0) {
                sscanf(argv[++i],"%i", &preRotate);
                if (abs(preRotate) != 90) {
                    printf("Cannot set --pre-rotate value other than -90 or 90, ignoring.\n");
                    preRotate = 0;
                }

            // --post-rotate
            } else if (strcmp(argv[i], "--post-rotate")==0) {
                sscanf(argv[++i],"%i", &postRotate);
                if (abs(postRotate) != 90) {
                    printf("Cannot set --post-rotate value other than -90 or 90, ignoring.\n");
                    postRotate = 0;
                }

            // --pre-mirror  -M
            } else if (strcmp(argv[i], "-M")==0 || strcmp(argv[i], "--pre-mirror")==0) {
                preMirror = parseDirections(argv[++i]); // s = "v", "v,h", "vertical,horizontal", ...

            // --post-mirror
            } else if (strcmp(argv[i], "--post-mirror")==0) {
                postMirror = parseDirections(argv[++i]);


            // --pre-mask
            } else if ( strcmp(argv[i], "--pre-mask")==0 && (preMaskCount<MAX_MASKS)) {
                left = -1;
                top = -1;
                right = -1;
                bottom = -1;
                sscanf(argv[++i],"%i,%i,%i,%i", &left, &top, &right, &bottom); // x1, y1, x2, y2
                preMask[preMaskCount][LEFT] = left;
                preMask[preMaskCount][TOP] = top;
                preMask[preMaskCount][RIGHT] = right;
                preMask[preMaskCount][BOTTOM] = bottom;
                preMaskCount++;


            // --mask-point  -p
            } else if ((strcmp(argv[i], "-p")==0 || strcmp(argv[i], "--mask-point")==0) && (pointCount<MAX_POINTS)) {
                x = -1;
                y = -1;
                sscanf(argv[++i],"%i,%i", &x, &y);
                point[pointCount][X] = x;
                point[pointCount][Y] = y;
                pointCount++;


            // --mask  -m    
            } else if ((strcmp(argv[i], "-m")==0 || strcmp(argv[i], "--mask")==0) && (maskCount<MAX_MASKS)) {
                left = -1;
                top = -1;
                right = -1;
                bottom = -1;
                sscanf(argv[++i],"%i,%i,%i,%i", &left, &top, &right, &bottom); // x1, y1, x2, y2
                mask[maskCount][LEFT] = left;
                mask[maskCount][TOP] = top;
                mask[maskCount][RIGHT] = right;
                mask[maskCount][BOTTOM] = bottom;
                maskValid[maskCount] = TRUE;
                maskCount++;


            // --wipe  -W    
            } else if ((strcmp(argv[i], "-W")==0 || strcmp(argv[i], "--wipe")==0) && (wipeCount < MAX_MASKS)) {
                left = -1;
                top = -1;
                right = -1;
                bottom = -1;
                sscanf(argv[++i],"%i,%i,%i,%i", &left, &top, &right, &bottom); // x1, y1, x2, y2
                wipe[wipeCount][LEFT] = left;
                wipe[wipeCount][TOP] = top;
                wipe[wipeCount][RIGHT] = right;
                wipe[wipeCount][BOTTOM] = bottom;
                wipeCount++;

            // ---pre-wipe
            } else if ((strcmp(argv[i], "--pre-wipe")==0) && (preWipeCount < MAX_MASKS)) {
                left = -1;
                top = -1;
                right = -1;
                bottom = -1;
                sscanf(argv[++i],"%i,%i,%i,%i", &left, &top, &right, &bottom); // x1, y1, x2, y2
                preWipe[preWipeCount][LEFT] = left;
                preWipe[preWipeCount][TOP] = top;
                preWipe[preWipeCount][RIGHT] = right;
                preWipe[preWipeCount][BOTTOM] = bottom;
                preWipeCount++;

            // ---post-wipe
            } else if ((strcmp(argv[i], "--post-wipe")==0) && (postWipeCount < MAX_MASKS)) {
                left = -1;
                top = -1;
                right = -1;
                bottom = -1;
                sscanf(argv[++i],"%i,%i,%i,%i", &left, &top, &right, &bottom); // x1, y1, x2, y2
                postWipe[postWipeCount][LEFT] = left;
                postWipe[postWipeCount][TOP] = top;
                postWipe[postWipeCount][RIGHT] = right;
                postWipe[postWipeCount][BOTTOM] = bottom;
                postWipeCount++;

            // --middle-wipe -mw
            } else if (strcmp(argv[i], "-mw")==0 || strcmp(argv[i], "--middle-wipe")==0) {
                //sscanf(argv[++i],"%i", &middleWipe);
                parseInts(argv[++i], middleWipe);


            // --border  -B
            } else if ((strcmp(argv[i], "-B")==0 || strcmp(argv[i], "--border")==0)) {
                sscanf(argv[++i],"%i,%i,%i,%i", &border[LEFT], &border[TOP], &border[RIGHT], &border[BOTTOM]);
                //noBorderScanMultiIndexCount = -1; // disable auto-detection of borders

            // --pre-border
            } else if (strcmp(argv[i], "--pre-border")==0) {
                sscanf(argv[++i],"%i,%i,%i,%i", &preBorder[LEFT], &preBorder[TOP], &preBorder[RIGHT], &preBorder[BOTTOM]);

            // --post-border
            } else if (strcmp(argv[i], "--post-border")==0) {
                sscanf(argv[++i],"%i,%i,%i,%i", &postBorder[LEFT], &postBorder[TOP], &postBorder[RIGHT], &postBorder[BOTTOM]);


            // --no-blackfilter
            } else if (strcmp(argv[i], "--no-blackfilter")==0) {
                //blackfilterScanDirections = 0;
                parseMultiIndex(&i, argv, noBlackfilterMultiIndex, &noBlackfilterMultiIndexCount);

            // --blackfilter-scan-direction  -bn
            } else if (strcmp(argv[i], "-bn")==0 || strcmp(argv[i], "--blackfilter-scan-direction")==0) {
                blackfilterScanDirections = parseDirections(argv[++i]);

            // --blackfilter-scan-size  -bs
            } else if (strcmp(argv[i], "-bs")==0 || strcmp(argv[i], "--blackfilter-scan-size")==0) {
                parseInts(argv[++i], blackfilterScanSize);

            // --blackfilter-scan-depth  -bd
            } else if (strcmp(argv[i], "-bd")==0 || strcmp(argv[i], "--blackfilter-scan-depth")==0) {
                parseInts(argv[++i], blackfilterScanDepth);

            // --blackfilter-scan-step  -bp
            } else if (strcmp(argv[i], "-bp")==0 || strcmp(argv[i], "--blackfilter-scan-step")==0) {
                parseInts(argv[++i], blackfilterScanStep);

            // --blackfilter-scan-threshold  -bt   
            } else if (strcmp(argv[i], "-bt")==0 || strcmp(argv[i], "--blackfilter-scan-threshold")==0) {
                sscanf(argv[++i], "%f", &blackfilterScanThreshold);

            // --blackfilter-intensity  -bi
            } else if (strcmp(argv[i], "-bi")==0 || strcmp(argv[i], "--blackfilter-intensity")==0) {
                sscanf(argv[++i], "%i", &blackfilterIntensity);


            // --no-noisefilter
            } else if (strcmp(argv[i], "--no-noisefilter")==0) {
                //noisefilterIntensity = 0;
                parseMultiIndex(&i, argv, noNoisefilterMultiIndex, &noNoisefilterMultiIndexCount);

            // --noisefilter-intensity  -ni 
            } else if (strcmp(argv[i], "-ni")==0 || strcmp(argv[i], "--noisefilter-intensity")==0) {
                sscanf(argv[++i], "%i", &noisefilterIntensity);


            // --no-blurfilter
            } else if (strcmp(argv[i], "--no-blurfilter")==0) {
                parseMultiIndex(&i, argv, noBlurfilterMultiIndex, &noBlurfilterMultiIndexCount);

            // --blurfilter-size  -ls
            } else if (strcmp(argv[i], "-ls")==0 || strcmp(argv[i], "--blurfilter-size")==0) {
                parseInts(argv[++i], blurfilterScanSize);

            // --blurfilter-step  -lp
            } else if (strcmp(argv[i], "-lp")==0 || strcmp(argv[i], "--blurfilter-step")==0) {
                parseInts(argv[++i], blurfilterScanStep);

            // --blurfilter-intensity  -li 
            } else if (strcmp(argv[i], "-li")==0 || strcmp(argv[i], "--blurfilter-intensity")==0) {
                sscanf(argv[++i], "%f", &blurfilterIntensity);


            // --no-grayfilter
            } else if (strcmp(argv[i], "--no-grayfilter")==0) {
                parseMultiIndex(&i, argv, noGrayfilterMultiIndex, &noGrayfilterMultiIndexCount);

            // --grayfilter-size  -gs
            } else if (strcmp(argv[i], "-gs")==0 || strcmp(argv[i], "--grayfilter-size")==0) {
                parseInts(argv[++i], grayfilterScanSize);

            // --grayfilter-step  -gp
            } else if (strcmp(argv[i], "-gp")==0 || strcmp(argv[i], "--grayfilter-step")==0) {
                parseInts(argv[++i], grayfilterScanStep);

            // --grayfilter-threshold  -gt 
            } else if (strcmp(argv[i], "-gt")==0 || strcmp(argv[i], "--grayfilter-threshold")==0) {
                sscanf(argv[++i], "%f", &grayfilterThreshold);


            // --no-mask-scan
            } else if (strcmp(argv[i], "--no-mask-scan")==0) {
                //maskScanDirections = 0;
                parseMultiIndex(&i, argv, noMaskScanMultiIndex, &noMaskScanMultiIndexCount);

            // --mask-scan-direction  -mn
            } else if (strcmp(argv[i], "-mn")==0 || strcmp(argv[i], "--mask-scan-direction")==0) {
                maskScanDirections = parseDirections(argv[++i]);

            // --mask-scan-size  -ms
            } else if (strcmp(argv[i], "-ms")==0 || strcmp(argv[i], "--mask-scan-size")==0) {
                parseInts(argv[++i], maskScanSize);

            // --mask-scan-depth  -md
            } else if (strcmp(argv[i], "-md")==0 || strcmp(argv[i], "--mask-scan-depth")==0) {
                parseInts(argv[++i], maskScanDepth);

            // --mask-scan-step  -mp
            } else if (strcmp(argv[i], "-mp")==0 || strcmp(argv[i], "--mask-scan-step")==0) {
                parseInts(argv[++i], maskScanStep);

            // --mask-scan-threshold  -mt   
            } else if (strcmp(argv[i], "-mt")==0 || strcmp(argv[i], "--mask-scan-threshold")==0) {
                //sscanf(argv[++i], "%f", &maskScanThreshold);
                parseFloats(argv[++i], maskScanThreshold);

            // --mask-scan-minimum  -mm
            } else if (strcmp(argv[i], "-mm")==0 || strcmp(argv[i], "--mask-scan-minimum")==0) {
                sscanf(argv[++i],"%i,%i", &maskScanMinimum[WIDTH], &maskScanMinimum[HEIGHT]);

            // --mask-color
            } else if (strcmp(argv[i], "-mc")==0 || strcmp(argv[i], "--mask-color")==0) {
                sscanf(argv[++i],"%i", &maskColor);


            // --no-mask-center
            } else if (strcmp(argv[i], "--no-mask-center")==0) {
                //maskcenter = FALSE;
                parseMultiIndex(&i, argv, noMaskCenterMultiIndex, &noMaskCenterMultiIndexCount);


            // --no-deskew
            } else if (strcmp(argv[i], "--no-deskew")==0) {
                parseMultiIndex(&i, argv, noDeskewMultiIndex, &noDeskewMultiIndexCount);

            // --deskew-scan-direction  -dn
            } else if (strcmp(argv[i], "-dn")==0 || strcmp(argv[i], "--deskew-scan-direction")==0) {
                deskewScanDirections = parseDirections(argv[++i]);

            // --deskew-scan-size  -ds
            } else if (strcmp(argv[i], "-ds")==0 || strcmp(argv[i], "--deskew-scan-size")==0) {
                sscanf(argv[++i],"%i", &deskewScanSize);

            // --deskew-scan-depth  -dd
            } else if (strcmp(argv[i], "-dd")==0 || strcmp(argv[i], "--deskew-scan-depth")==0) {
                sscanf(argv[++i],"%f", &deskewScanDepth);

            // --deskew-scan-range  -dr
            } else if (strcmp(argv[i], "-dr")==0 || strcmp(argv[i], "--deskew-scan-range")==0) {
                sscanf(argv[++i],"%f", &deskewScanRange);

            // --deskew-scan-step  -dp
            } else if (strcmp(argv[i], "-dp")==0 || strcmp(argv[i], "--deskew-scan-step")==0) {
                sscanf(argv[++i],"%f", &deskewScanStep);

            // --deskew-scan-threshold  -dt
            } else if (strcmp(argv[i], "-dt")==0 || strcmp(argv[i], "--deskew-scan-threshold")==0) {
                sscanf(argv[++i],"%f", &deskewScanThreshold);

            // --deskew-scan-deviation  -dv
            } else if (strcmp(argv[i], "-dv")==0 || strcmp(argv[i], "--deskew-scan-deviation")==0) {
                sscanf(argv[++i],"%f", &deskewScanDeviation);


            // --no-border-scan
            } else if (strcmp(argv[i], "--no-border-scan")==0) {
                parseMultiIndex(&i, argv, noBorderScanMultiIndex, &noBorderScanMultiIndexCount);

            // --border-scan-direction  -Bn
            } else if (strcmp(argv[i], "-Bn")==0 || strcmp(argv[i], "--border-scan-direction")==0) {
                borderScanDirections = parseDirections(argv[++i]);

            // --border-scan-size  -Bs
            } else if (strcmp(argv[i], "-Bs")==0 || strcmp(argv[i], "--border-scan-size")==0) {
                parseInts(argv[++i], borderScanSize);

            // --border-scan-step  -Bp
            } else if (strcmp(argv[i], "-Bp")==0 || strcmp(argv[i], "--border-scan-step")==0) {
                parseInts(argv[++i], borderScanStep);

            // --border-scan-threshold  -Bt   
            } else if (strcmp(argv[i], "-Bt")==0 || strcmp(argv[i], "--border-scan-threshold")==0) {
                parseInts(argv[++i], borderScanThreshold);


            // --no-border-center
            } else if (strcmp(argv[i], "--no-border-center")==0) {
                parseMultiIndex(&i, argv, noBorderCenterMultiIndex, &noBorderCenterMultiIndexCount);


            // --no-wipe
            } else if (strcmp(argv[i], "--no-wipe")==0) {
                parseMultiIndex(&i, argv, noWipeMultiIndex, &noWipeMultiIndexCount);


            // --no-border
            } else if (strcmp(argv[i], "--no-border")==0) {
                parseMultiIndex(&i, argv, noBorderMultiIndex, &noBorderMultiIndexCount);


            // --white-treshold
            } else if (strcmp(argv[i], "-w")==0 || strcmp(argv[i], "--white-threshold")==0) {
                sscanf(argv[++i],"%f", &whiteThreshold);
            // --black-treshold
            } else if (strcmp(argv[i], "-b")==0 || strcmp(argv[i], "--black-threshold")==0) {
                sscanf(argv[++i],"%f", &blackThreshold);

            // --test-only  -T
            } else if (strcmp(argv[i], "-T")==0 || strcmp(argv[i], "--test-only")==0) {
                writeoutput = FALSE;

            // --no-qpixels
            } else if (strcmp(argv[i], "--no-qpixels")==0) {
                qpixels = FALSE;

            // --no-multi-pages
            } else if (strcmp(argv[i], "--no-multi-pages")==0) {
                multisheets = FALSE;

            // --type  -t
            } else if (strcmp(argv[i], "-t")==0 || strcmp(argv[i], "--type")==0) { 
                outputTypeName = argv[++i];

            // --quiet  -q
            } else if (strcmp(argv[i], "-q")==0  || strcmp(argv[i], "--quiet")==0) {
                verbose = VERBOSE_QUIET;

            // --verbose  -v
            } else if (strcmp(argv[i], "-v")==0  || strcmp(argv[i], "--verbose")==0) {
                verbose = VERBOSE_NORMAL;

            // -vv
            } else if (strcmp(argv[i], "-vv")==0) {
                verbose = VERBOSE_MORE;

            // --debug -vvv (undocumented)
            } else if (strcmp(argv[i], "-vvv")==0 || strcmp(argv[i], "--debug")==0) {
                verbose = VERBOSE_DEBUG;

            // --debug-save -vvvv (undocumented)
            } else if (strcmp(argv[i], "-vvvv")==0 || strcmp(argv[i], "--debug-save")==0) {
                verbose = VERBOSE_DEBUG_SAVE;

            // unkown parameter            
            } else {
                printf("*** error: Unknown parameter '%s'.\n", argv[i]);
                printf(HELP);
                return 1;
            }
            i++;
        }

        // welcome message
        if ((nr == startSheet) && (verbose >= VERBOSE_NORMAL)) {
            printf(WELCOME, VERSION);
        }
    
        // get filenames    
        if (i < argc) {
            inputFilename = argv[i++];
        } else {
            printf("*** error: No input file specified.\n");
            printf(HELP);
            return 1;
        }
        if (i < argc) {
            outputFilename = argv[i++];
        } else {
            if (writeoutput) {
                printf("*** error: No output file specified.\n");
                printf(HELP);
                return 1;
            } // else ignore
        }

        // multi-pages?
        if ( multisheets && (strchr(inputFilename, '%') != 0)) { // might already have been disabled by option (multisheets==FALSE)
            //nop, remain TRUE
        } else {
            multisheets = FALSE;
            startSheet = endSheet = 1;
        }

        // get filenames
        sprintf(inputFilenameResolved, inputFilename, nr);
        sprintf(outputFilenameResolved, outputFilename, nr);

        // test if input file exists        
        if ( multisheets && (!fileExists(inputFilenameResolved)) ) {
            if (nr == startSheet) { // only an error if first file not found, otherwise regular end of multisheet processing
                printf("*** error: Input file %s not found.\n", inputFilenameResolved);
            }
            endSheet = nr - 1; // exit for-loop

        } else {

            // --- process single sheet --------------------------------------
            
            if (isInMultiIndex(nr, sheetMultiIndex, sheetMultiIndexCount) && (!isInMultiIndex(nr, excludeMultiIndex, excludeMultiIndexCount))) {

                if (verbose >= VERBOSE_NORMAL) {
                    printf("\n-------------------------------------------------------------------------------\n");
                }
                if (verbose > VERBOSE_QUIET) {
                    if (multisheets) {
                        printf("Processing sheet #%i: %s -> %s\n", nr, inputFilenameResolved, outputFilenameResolved);
                    } else {
                        printf("Processing sheet: %s -> %s\n", inputFilenameResolved, outputFilenameResolved);
                    }
                }

                // load image
                success = loadImage(inputFilenameResolved, &buffer, &width, &height, &inputType);
                if (!success) {
                    if (nr == startSheet) {
                        printf("*** error: Cannot load image %s.\n", inputFilenameResolved);
                    }
                    exitCode = 2;

                } else { // image has been loaded successfully

                    // handle file types
                    if (inputType == PBM) {
                        inputTypeName = "pbm";
                    } else if (inputType == PGM) {
                        inputTypeName = "pgm";
                    }
                    if (outputTypeName==NULL) { // set output type to be as input type, if not explicitly set by user
                        outputTypeName = inputTypeName;
                        outputType = inputType;
                    } else { // parse user-set output type string
                        if (strcmp(outputTypeName, "pbm")==0) {
                            outputType = PBM;
                        } else if (strcmp(outputTypeName, "pgm")==0) {
                            outputType = PGM;
                        } else {
                            printf("*** error: Output file format '%s' is not known.\n", outputTypeName);
                            return 2;
                        }
                    }

                    if (verbose >= VERBOSE_DEBUG) {
                        startTime = clock();
                    }

                    // pre-rotating
                    if (preRotate != 0) {
                        if (verbose>=VERBOSE_NORMAL) {
                            printf("pre-rotating %i degrees.\n", preRotate);
                        }
                        if (preRotate == 90) {
                            flipRotate(1, &buffer, &width, &height, inputType);
                        } else if (preRotate == -90) {
                            flipRotate(-1, &buffer, &width, &height, inputType);
                        }
                    }

                    // pre-mirroring
                    if (preMirror != 0) {
                        if (verbose>=VERBOSE_NORMAL) {
                            printf("pre-mirroring ");
                            printDirections(preMirror);
                        }
                        mirror(preMirror, buffer, width, height, inputType);
                    }

                    // pre-masking
                    if (preMaskCount > 0) {
                        if (verbose>=VERBOSE_NORMAL) {
                            printf("pre-masking\n ");
                        }
                        applyMasks(preMask, preMaskCount, maskColor, buffer, width, height, inputType);
                    }

                    // handle sheet layout
                    if (layout == LAYOUT_SINGLE) {
                        // set middle of sheet as single starting point for mask detection
                        point[0][X] = width / 2;
                        point[0][Y] = height / 2;
                        pointCount = 1;
                        maskScanMaximum[WIDTH] = width;
                        maskScanMaximum[HEIGHT] = height;
                    } else if (layout == LAYOUT_DOUBLE) {
                        // set two middle of left/right side of sheet as starting points for mask detection
                        point[0][X] = width / 4;
                        point[0][Y] = height / 2;
                        point[1][X] = width - width / 4;
                        point[1][Y] = height / 2;
                        pointCount = 2;
                        maskScanMaximum[WIDTH] = width / 2;
                        maskScanMaximum[HEIGHT] = height;
                        if (middleWipe[0] > 0 || middleWipe[1] > 0) { // left, right
                            wipe[wipeCount][LEFT] = width / 2 - middleWipe[0];
                            wipe[wipeCount][TOP] = 0;
                            wipe[wipeCount][RIGHT] =  width / 2 + middleWipe[1];
                            wipe[wipeCount][BOTTOM] = height - 1;
                            wipeCount++;
                        }
                    }
                    // if maskScanMaximum still unset (no --layout specified), set to full sheet size now
                    if (maskScanMinimum[WIDTH] == -1) {
                        maskScanMaximum[WIDTH] = width;
                    }
                    if (maskScanMinimum[HEIGHT] == -1) {
                        maskScanMaximum[HEIGHT] = height;
                    }

                    // --- verbose message, parameters and input are now known ---
                    if (verbose >= VERBOSE_MORE) {
                        if (layout != LAYOUT_NONE) {
                            if (layout == LAYOUT_SINGLE) {
                                layoutStr = "single";
                            } else if (layout == LAYOUT_DOUBLE) {
                                layoutStr = "double";
                            }
                            printf("layout: %s\n", layoutStr);
                        }

                        if (preRotate != 0) {
                            printf("pre-rotate: %i\n", preRotate);
                        }
                        if (preMirror != 0) {
                            printf("pre-mirror: ");
                            printDirections(preMirror);
                        }
                        if (preWipeCount > 0) {
                            printf("pre-wipe: ");
                            for (i = 0; i < preWipeCount; i++) {
                                printf("[%i,%i,%i,%i] ",preWipe[i][LEFT],preWipe[i][TOP],preWipe[i][RIGHT],preWipe[i][BOTTOM]);
                            }
                            printf("\n");
                        }
                        if (preBorder[LEFT]!=0 || preBorder[TOP]!=0 || preBorder[RIGHT]!=0 || preBorder[BOTTOM]!=0) {
                            printf("pre-border: [%i,%i,%i,%i]\n", preBorder[LEFT], preBorder[TOP], preBorder[RIGHT], preBorder[BOTTOM]);
                        }
                        if (preMaskCount > 0) {
                            printf("pre-masking: ");
                            for (i = 0; i < preMaskCount; i++) {
                                printf("[%i,%i,%i,%i] ",preMask[i][LEFT],preMask[i][TOP],preMask[i][RIGHT],preMask[i][BOTTOM]);
                            }
                            printf("\n");
                        }
                        if (noBlackfilterMultiIndexCount != -1) {
                            printf("blackfilter-scan-direction: ");
                            printDirections(blackfilterScanDirections);
                            printf("blackfilter-scan-size: ");
                            printInts(blackfilterScanSize);
                            printf("blackfilter-scan-depth: ");
                            printInts(blackfilterScanDepth);
                            printf("blackfilter-scan-step: ");
                            printInts(blackfilterScanStep);
                            printf("blackfilter-scan-threshold: %f\n", blackfilterScanThreshold);
                            printf("blackfilter-intensity: %i\n", blackfilterIntensity);
                            if (noBlackfilterMultiIndexCount > 0) {
                                printf("blackfilter DISABLED for sheets: ");
                                printMultiIndex(noBlackfilterMultiIndex, noBlackfilterMultiIndexCount);
                            }
                        } else {
                            printf("blackfilter DISABLED for all sheets.\n");
                        }
                        if (noNoisefilterMultiIndexCount != -1) {
                            printf("noisefilter-intensity: %i\n", noisefilterIntensity);
                            if (noNoisefilterMultiIndexCount > 0) {
                                printf("noisefilter DISABLED for sheets: ");
                                printMultiIndex(noNoisefilterMultiIndex, noNoisefilterMultiIndexCount);
                            }
                        } else {
                            printf("noisefilter DISABLED for all sheets.\n");
                        }
                        if (noBlurfilterMultiIndexCount != -1) {
                            printf("blurfilter-size: ");
                            printInts(blurfilterScanSize);
                            printf("blurfilter-step: ");
                            printInts(blurfilterScanStep);
                            printf("blurfilter-intensity: %f\n", blurfilterIntensity);
                            if (noBlurfilterMultiIndexCount > 0) {
                                printf("blurfilter DISABLED for sheets: ");
                                printMultiIndex(noBlurfilterMultiIndex, noBlurfilterMultiIndexCount);
                            }
                        } else {
                            printf("blurfilter DISABLED for all sheets.\n");
                        }
                        if (noGrayfilterMultiIndexCount != -1) {
                            printf("grayfilter-size: ");
                            printInts(grayfilterScanSize);
                            printf("grayfilter-step: ");
                            printInts(grayfilterScanStep);
                            printf("grayfilter-threshold: %f\n", grayfilterThreshold);
                            if (noGrayfilterMultiIndexCount > 0) {
                                printf("grayfilter DISABLED for sheets: ");
                                printMultiIndex(noGrayfilterMultiIndex, noGrayfilterMultiIndexCount);
                            }
                        } else {
                            printf("grayfilter DISABLED for all sheets.\n");
                        }
                        if (noMaskScanMultiIndexCount != -1) {
                            printf("mask points: ");
                            for (i = 0; i < pointCount; i++) {
                                printf("%i,%i ",point[i][X],point[i][Y]);
                            }
                            printf("\n");
                            printf("mask-scan-direction: ");
                            printDirections(maskScanDirections);
                            printf("mask-scan-size: ");
                            printInts(maskScanSize);
                            printf("mask-scan-depth: ");
                            printInts(maskScanDepth);
                            printf("mask-scan-step: ");
                            printInts(maskScanStep);
                            printf("mask-scan-threshold: ");//%f\n", maskScanThreshold);
                            printFloats(maskScanThreshold);
                            printf("mask-scan-minimum: [%i,%i]\n", maskScanMinimum[WIDTH], maskScanMinimum[HEIGHT]);
                            printf("mask-scan-maximum: [%i,%i]\n", maskScanMaximum[WIDTH], maskScanMaximum[HEIGHT]);
                            printf("mask-color: %i\n", maskColor);
                            if (noMaskScanMultiIndexCount > 0) {
                                printf("mask-scan DISABLED for sheets: ");
                                printMultiIndex(noMaskScanMultiIndex, noMaskScanMultiIndexCount);
                            }
                        } else {
                            printf("mask-scan DISABLED for all sheets.\n");
                        }
                        if (noDeskewMultiIndexCount != -1) {
                            printf("deskew-scan-direction: ");
                            printDirections(deskewScanDirections);
                            printf("deskew-scan-size: %i\n", deskewScanSize);
                            printf("deskew-scan-depth: %f\n", deskewScanDepth);
                            printf("deskew-scan-range: %f\n", deskewScanRange);
                            printf("deskew-scan-step: %f\n", deskewScanStep);
                            //printf("deskew-scan-threshold: %f\n", deskewScanThreshold);
                            printf("deskew-scan-deviation: %f\n", deskewScanDeviation);
                            if (qpixels==FALSE) {
                                printf("qpixel-coding DISABLED.\n");
                            }
                            if (noDeskewMultiIndexCount > 0) {
                                printf("deskew-scan DISABLED for sheets: ");
                                printMultiIndex(noDeskewMultiIndex, noDeskewMultiIndexCount);
                            }
                        } else {
                            printf("deskew-scan DISABLED for all sheets.\n");
                        }
                        if (noWipeMultiIndexCount != -1) {
                            if (wipeCount > 0) {
                                printf("wipe areas: ");
                                for (i = 0; i < wipeCount; i++) {
                                    printf("[%i,%i,%i,%i] ", wipe[i][LEFT], wipe[i][TOP], wipe[i][RIGHT], wipe[i][BOTTOM]);
                                }
                                printf("\n");
                            }
                        } else {
                            printf("wipe DISABLED for all sheets.\n");
                        }
                        if (middleWipe[0] > 0 || middleWipe[1] > 0) {
                            printf("middle-wipe (l,r): %i,%i\n", middleWipe[0], middleWipe[1]);
                        }
                        if (noBorderMultiIndexCount != -1) {
                            if (border[LEFT]!=0 || border[TOP]!=0 || border[RIGHT]!=0 || border[BOTTOM]!=0) {
                                printf("explicit border: [%i,%i,%i,%i]\n", border[LEFT], border[TOP], border[RIGHT], border[BOTTOM]);
                            }
                        } else {
                            printf("border DISABLED for all sheets.\n");
                        }
                        if (noBorderScanMultiIndexCount != -1) {
                            printf("border-scan-direction: ");
                            printDirections(borderScanDirections);
                            printf("border-scan-size: ");
                            printInts(borderScanSize);
                            printf("border-scan-step: ");
                            printInts(borderScanStep);
                            printf("border-scan-threshold: ");//%f\n", maskScanThreshold);
                            printInts(borderScanThreshold);
                            if (noBorderScanMultiIndexCount > 0) {
                                printf("border-scan DISABLED for sheets: ");
                                printMultiIndex(noBorderScanMultiIndex, noBorderScanMultiIndexCount);
                            }
                        } else {
                            printf("border-scan DISABLED for all sheets.\n");
                        }
                        if (postWipeCount > 0) {
                            printf("post-wipe: ");
                            for (i = 0; i < postWipeCount; i++) {
                                printf("[%i,%i,%i,%i] ",postWipe[i][LEFT],postWipe[i][TOP],postWipe[i][RIGHT],postWipe[i][BOTTOM]);
                            }
                            printf("\n");
                        }
                        if (postBorder[LEFT]!=0 || postBorder[TOP]!=0 || postBorder[RIGHT]!=0 || postBorder[BOTTOM]!=0) {
                            printf("post-border: [%i,%i,%i,%i]\n", postBorder[LEFT], postBorder[TOP], postBorder[RIGHT], postBorder[BOTTOM]);
                        }
                        if (postMirror != 0) {
                            printf("post-mirror: ");
                            printDirections(postMirror);
                        }
                        if (postRotate != 0) {
                            printf("post-rotate: %i\n", postRotate);
                        }
                        //if (ignoreMultiIndexCount > 0) {
                        //    printf("EXCLUDE sheets: ");
                        //    printMultiIndex(ignoreMultiIndex, ignoreMultiIndexCount);
                        //}
                        printf("white-threshold: %f\n", whiteThreshold);
                        printf("black-threshold: %f\n", blackThreshold);
                        printf("input-file:  %s (type %s, %i x %i pixels)\n", inputFilenameResolved, inputTypeName, width, height);
                        printf("output-file: %s (type %s, %i x %i pixels)\n", outputFilenameResolved, outputTypeName, width, height);
                        printf("...\n");
                    }

                    // --- process image data --------------------------------

                    // pre-wipe
                    if (!isExcluded(nr, noWipeMultiIndex, noWipeMultiIndexCount, ignoreMultiIndex, ignoreMultiIndexCount)) {
                        applyWipes(preWipe, preWipeCount, WHITE, buffer, width, height, inputType);
                    }

                    // pre-border
                    if (!isExcluded(nr, noBorderMultiIndex, noBorderMultiIndexCount, ignoreMultiIndex, ignoreMultiIndexCount)) {
                        applyBorder(preBorder, WHITE, buffer, width, height, inputType);
                    }

                    // black area filter
                    if (!isExcluded(nr, noBlackfilterMultiIndex, noBlackfilterMultiIndexCount, ignoreMultiIndex, ignoreMultiIndexCount)) {
                        saveDebug("./_before-blackfilter.pgm", buffer, width, height);
                        blackfilter(blackfilterScanDirections, blackfilterScanSize, blackfilterScanDepth, blackfilterScanStep, blackfilterScanThreshold, blackfilterIntensity, blackThreshold, buffer, width, height, inputType);
                        saveDebug("./_after-blackfilter.pgm", buffer, width, height);
                    } else {
                        if (verbose >= VERBOSE_NORMAL) {
                            printf("+ blackfilter DISABLED for sheet %i\n", nr);
                        }
                    }

                    // noise filter
                    if (!isExcluded(nr, noNoisefilterMultiIndex, noNoisefilterMultiIndexCount, ignoreMultiIndex, ignoreMultiIndexCount)) {
                        if (verbose >= VERBOSE_NORMAL) {
                            printf("noise-filter ...");
                        }
                        saveDebug("./_before-noisefilter.pgm", buffer, width, height);
                        filterResult = noisefilter(noisefilterIntensity, whiteThreshold, buffer, width, height, inputType);
                        saveDebug("./_after-noisefilter.pgm", buffer, width, height);
                        if (verbose >= VERBOSE_NORMAL) {
                            printf(" deleted %i clusters.\n", filterResult);
                        }
                    } else {
                        if (verbose >= VERBOSE_NORMAL) {
                            printf("+ noisefilter DISABLED for sheet %i\n", nr);
                        }
                    }

                    // blur filter
                    if (!isExcluded(nr, noBlurfilterMultiIndex, noBlurfilterMultiIndexCount, ignoreMultiIndex, ignoreMultiIndexCount)) {
                        if (verbose >= VERBOSE_NORMAL) {
                            printf("blur-filter...");
                        }
                        saveDebug("./_before-blurfilter.pgm", buffer, width, height);
                        filterResult = blurfilter(blurfilterScanSize, blurfilterScanStep, blurfilterIntensity, whiteThreshold, buffer, width, height, inputType);
                        saveDebug("./_after-blurfilter.pgm", buffer, width, height);
                        if (verbose >= VERBOSE_NORMAL) {
                            printf(" deleted %i pixels.\n", filterResult);
                        }
                    } else {
                        if (verbose >= VERBOSE_NORMAL) {
                            printf("+ blurfilter DISABLED for sheet %i\n", nr);
                        }
                    }

                    // mask-detection
                    if (!isExcluded(nr, noMaskScanMultiIndex, noMaskScanMultiIndexCount, ignoreMultiIndex, ignoreMultiIndexCount)) {
                        maskCount = detectMasks(mask, maskValid, point, pointCount, maskScanDirections, maskScanSize, maskScanDepth, maskScanStep, maskScanThreshold, maskScanMinimum, maskScanMaximum, buffer, width, height, inputType);
                    } else {
                        if (verbose >= VERBOSE_NORMAL) {
                            printf("+ mask-scan DISABLED for sheet %i\n", nr);
                        }
                    }

                    // permamently apply masks
                    if (maskCount > 0) {
                        saveDebug("./_before-masking.pgm", buffer, width, height);
                        applyMasks(mask, maskCount, maskColor, buffer, width, height, inputType);
                        saveDebug("./_after-masking.pgm", buffer, width, height);
                    }

                    // gray filter
                    if (!isExcluded(nr, noGrayfilterMultiIndex, noGrayfilterMultiIndexCount, ignoreMultiIndex, ignoreMultiIndexCount)) {
                        if (verbose >= VERBOSE_NORMAL) {
                            printf("gray-filter...");
                        }
                        saveDebug("./_before-grayfilter.pgm", buffer, width, height);
                        filterResult = grayfilter(grayfilterScanSize, grayfilterScanStep, grayfilterThreshold, blackThreshold, buffer, width, height, inputType);
                        saveDebug("./_after-grayfilter.pgm", buffer, width, height);
                        if (verbose >= VERBOSE_NORMAL) {
                            printf(" deleted %i pixels.\n", filterResult);
                        }
                    } else {
                        if (verbose >= VERBOSE_NORMAL) {
                            printf("+ grayfilter DISABLED for sheet %i\n", nr);
                        }
                    }

                    // rotation-detection
                    if ((!isExcluded(nr, noDeskewMultiIndex, noDeskewMultiIndexCount, ignoreMultiIndex, ignoreMultiIndexCount))) {
                        saveDebug("./_before-deskew.pgm", buffer, width, height);
                        originalBuffer = buffer;
                        // convert to qpixels
                        if (qpixels==TRUE) {
                            if (verbose>=VERBOSE_NORMAL) {
                                printf("converting to qpixels.\n");
                            }
                            qpixelBuffer = (char*)malloc(width*2 * height*2);
                            convertToQpixels(buffer, width, height, inputType, qpixelBuffer);
                            buffer = qpixelBuffer;
                            q = 2; // qpixel-factor for coordinates in both directions
                        } else {
                            q = 1;
                        }

                        // detect masks again, we may get more precise results now after first masking and grayfilter
                        if (!isExcluded(nr, noMaskScanMultiIndex, noMaskScanMultiIndexCount, ignoreMultiIndex, ignoreMultiIndexCount)) {
                            maskCount = detectMasks(mask, maskValid, point, pointCount, maskScanDirections, maskScanSize, maskScanDepth, maskScanStep, maskScanThreshold, maskScanMinimum, maskScanMaximum, originalBuffer, width, height, inputType);
                        } else {
                            if (verbose >= VERBOSE_NORMAL) {
                                printf("(mask-scan before deskewing disabled)\n", nr);
                            }
                        }

                        // auto-deskew each mask
                        for (i = 0; i < maskCount; i++) {

                            if ( maskValid[i] == TRUE ) { // point may have been invalidated if mask has not been auto-detected

                                // for rotation detection, original buffer is used (not qpixels)
                                saveDebug("./_before-deskew-detect.pgm", originalBuffer, width, height);
                                rotation = - detectRotation(deskewScanDirections, deskewScanRange, deskewScanStep, deskewScanSize, deskewScanDepth, deskewScanThreshold, deskewScanDeviation, mask[i][LEFT], mask[i][TOP], mask[i][RIGHT], mask[i][BOTTOM], originalBuffer, width, height, inputType);
                                saveDebug("./_after-deskew-detect.pgm", originalBuffer, width, height);

                                if (rotation != 0.0) {
                                    if (verbose>=VERBOSE_NORMAL) {
                                        printf("rotate (%i,%i): %f\n", point[i][X], point[i][Y], rotation);
                                    }
                                    rWidth = (mask[i][RIGHT]-mask[i][LEFT])*q;
                                    rHeight = (mask[i][BOTTOM]-mask[i][TOP])*q;
                                    rSize = rWidth * rHeight;

                                    rSource = (char*)malloc(rSize);
                                    r = (char*)malloc(rSize);

                                    // copy area to rotate into rSource
                                    copyBuffer(mask[i][LEFT]*q, mask[i][TOP]*q, rWidth, rHeight, buffer, width*q, height*q, inputType,
                                               0, 0, rSource, rWidth, rHeight, inputType);

                                    rotate(degreesToRadians(rotation), rSource, r, rWidth, rHeight, inputType);

                                    // copy result back into whole image
                                    copyBuffer(0, 0, rWidth, rHeight, r, rWidth, rHeight, inputType, 
                                               mask[i][LEFT]*q, mask[i][TOP]*q, buffer, width*q, height*q, inputType);

                                    free(rSource);
                                    free(r);
                                } else {
                                    if (verbose>=VERBOSE_NORMAL) {
                                        printf("rotate (%i,%i): -\n", point[i][X], point[i][Y]);
                                    }
                                }

                            }
                        } 

                        // convert back from qpixels
                        if (qpixels==TRUE) {
                            if (verbose>=VERBOSE_NORMAL) {
                                printf("converting back from qpixels.\n");
                            }
                            convertFromQpixels(qpixelBuffer, originalBuffer, width, height, inputType); //, qpixelDepth);
                            free(qpixelBuffer);
                            buffer = originalBuffer;
                        }
                        saveDebug("./_after-deskew.pgm", buffer, width, height);
                    } else {
                        if (verbose >= VERBOSE_NORMAL) {
                            printf("+ auto-rotation DISABLED for sheet %i\n", nr);
                        }
                    }

                    // auto-center masks on either single-page or double-page layout
                    if ( (!isExcluded(nr, noMaskCenterMultiIndex, noMaskCenterMultiIndexCount, ignoreMultiIndex, ignoreMultiIndexCount)) && (layout != LAYOUT_NONE) && (maskCount == pointCount) ) { // (maskCount==pointCount to make sure all masks had correctly been detected)
                        // perform auto-masking again to get more precise masks after rotation                    
                        if (!isExcluded(nr, noMaskScanMultiIndex, noMaskScanMultiIndexCount, ignoreMultiIndex, ignoreMultiIndexCount)) {
                            maskCount = detectMasks(mask, maskValid, point, pointCount, maskScanDirections, maskScanSize, maskScanDepth, maskScanStep, maskScanThreshold, maskScanMinimum, maskScanMaximum, buffer, width, height, inputType);
                        } else {
                            if (verbose >= VERBOSE_NORMAL) {
                                printf("(mask-scan before centering disabled)\n", nr);
                            }
                        }

                        saveDebug("./_before-centering.pgm", buffer, width, height);
                        // center masks on the sheet, according to their page position
                        for (i = 0; i < maskCount; i++) {
                            centerMask(point[i][X], point[i][Y], mask[i][LEFT], mask[i][TOP], mask[i][RIGHT], mask[i][BOTTOM], buffer, width, height, inputType);
                        }
                        saveDebug("./_after-centering.pgm", buffer, width, height);
                    } else {
                        if (verbose >= VERBOSE_NORMAL) {
                            printf("+ auto-centering DISABLED for sheet %i\n", nr);
                        }
                    }

                    // explicit wipe
                    if (!isExcluded(nr, noWipeMultiIndex, noWipeMultiIndexCount, ignoreMultiIndex, ignoreMultiIndexCount)) {
                        applyWipes(wipe, wipeCount, WHITE, buffer, width, height, inputType);
                    } else {
                        if (verbose >= VERBOSE_NORMAL) {
                            printf("+ wipe DISABLED for sheet %i\n", nr);
                        }
                    }

                    // explicit border
                    if (!isExcluded(nr, noBorderMultiIndex, noBorderMultiIndexCount, ignoreMultiIndex, ignoreMultiIndexCount)) {
                        applyBorder(border, WHITE, buffer, width, height, inputType);
                    } else {
                        if (verbose >= VERBOSE_NORMAL) {
                            printf("+ border DISABLED for sheet %i\n", nr);
                        }
                    }

                    // border-detection
                    if (!isExcluded(nr, noBorderScanMultiIndex, noBorderScanMultiIndexCount, ignoreMultiIndex, ignoreMultiIndexCount)) {
                        detectBorder(autoborder, borderScanDirections, borderScanSize, borderScanStep, borderScanThreshold, blackThreshold, buffer, width, height, inputType);
                        saveDebug("./_before-border.pgm", buffer, width, height);
                        applyBorder(autoborder, WHITE, buffer, width, height, inputType);
                        saveDebug("./_after-border.pgm", buffer, width, height);
                        // border-centering
                        if (!isExcluded(nr, noBorderCenterMultiIndex, noBorderCenterMultiIndexCount, ignoreMultiIndex, ignoreMultiIndexCount)) {
                            saveDebug("./_before-border-center.pgm", buffer, width, height);
                            centerBorder(autoborder, buffer, width, height, inputType);
                            saveDebug("./_after-border-center.pgm", buffer, width, height);
                        } else {
                            if (verbose >= VERBOSE_NORMAL) {
                                printf("+ border-centering DISABLED for sheet %i\n", nr);
                            }
                        }
                    } else {
                        if (verbose >= VERBOSE_NORMAL) {
                            printf("+ border-scan DISABLED for sheet %i\n", nr);
                        }
                    }

                    // post-wipe
                    if (!isExcluded(nr, noWipeMultiIndex, noWipeMultiIndexCount, ignoreMultiIndex, ignoreMultiIndexCount)) {
                        applyWipes(postWipe, postWipeCount, WHITE, buffer, width, height, inputType);
                    }

                    // post-border
                    if (!isExcluded(nr, noBorderMultiIndex, noBorderMultiIndexCount, ignoreMultiIndex, ignoreMultiIndexCount)) {
                        applyBorder(postBorder, WHITE, buffer, width, height, inputType);
                    }

                    // post-mirroring
                    if (postMirror != 0) {
                        if (verbose>=VERBOSE_NORMAL) {
                            printf("post-mirroring ");
                            printDirections(postMirror);
                        }
                        mirror(postMirror, buffer, width, height, inputType);
                    }

                    // post-rotating
                    if (postRotate != 0) {
                        if (verbose >= VERBOSE_NORMAL) {
                            printf("post-rotating %i degrees.\n", postRotate);
                        }
                        if (postRotate == 90) {
                            flipRotate(1, &buffer, &width, &height, inputType);
                        } else if (postRotate == -90) {
                            flipRotate(-1, &buffer, &width, &height, inputType);
                        }
                    }

                    if (verbose >= VERBOSE_DEBUG) {
                        endTime = clock();
                    }

                    // --- write output file ---

                    if (writeoutput == TRUE) {    
                        if (verbose >= VERBOSE_NORMAL) {
                            printf("writing output file.\n");
                        }
                        success = saveImage(outputFilenameResolved, buffer, width, height, outputType, blackThreshold);
                        if (success == FALSE) {
                            printf("*** error: Could not save image data to file %s.\n", outputFilenameResolved);
                            exitCode = 2;
                        }
                    }

                    free(buffer);    

                    if (verbose >= VERBOSE_DEBUG) {
                        if (startTime > endTime) { // clock overflow
                            endTime -= startTime; // "re-underflow" value again
                            startTime = 0;
                        }
                        time = endTime - startTime;
                        totalTime += time;
                        totalCount++;
                        printf("- processing time:  %f s\n", (float)time/CLOCKS_PER_SEC);
                    }
                }
            }
        }
    }
    if ((verbose >= VERBOSE_DEBUG) && (totalCount > 1)) {
       printf("- total processing time of all %i sheets:  %f s  (average:  %f s)\n", totalCount, (float)totalTime/CLOCKS_PER_SEC, (float)totalTime/totalCount/CLOCKS_PER_SEC);
    }
    return exitCode;
}
