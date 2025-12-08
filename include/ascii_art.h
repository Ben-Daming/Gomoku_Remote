#ifndef ASCII_ART_H
#define ASCII_ART_H
#ifdef __cplusplus
//唯一的全局变量区。后续开发若要进行整个局面级别的并发编程，为保险起见可以回退到老版本
extern "C" {
#endif

extern const char* ANGEL_COMMON[];
extern const char* ANGEL_SMILE[];
extern const int ANGEL_LINES;
extern const char* ANGEL_AFRAID[];
extern int ascii_face_flag;

void setAsciiFaceFlag(int flag);
int getAsciiFaceFlag(void);

void printAsciiArray(const char* arr[], int lines);

#ifdef __cplusplus
}
#endif

#endif 
