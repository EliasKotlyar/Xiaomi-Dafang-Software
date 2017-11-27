#ifndef _OV9724_H_
#define _OV9724_H_
int ov9724_read(struct v4l2_subdev *sd, unsigned short reg, unsigned char *value);

#endif
