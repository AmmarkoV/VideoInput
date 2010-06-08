#ifndef PNG_INTERFACE_H_INCLUDED
#define PNG_INTERFACE_H_INCLUDED

void read_png_file(char* file_name);
void png_file_metrics(void * frame,int png_width,int png_height,int png_color_type,int png_bit_depth);
void write_png_file(char* file_name);


#endif // PNG_INTERFACE_H_INCLUDED
