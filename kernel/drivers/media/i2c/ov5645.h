#ifndef _OV5645_H_
#define _OV5645_H_
int ov5645_read(struct v4l2_subdev *sd, unsigned short reg, unsigned char *value);

#endif
