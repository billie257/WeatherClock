#ifndef __IMAGE_H__
#define __IMAGE_H__

#include <stdint.h>

typedef struct
{
		uint16_t width;
		uint16_t height;
		const uint8_t *data;
} image_t;

extern const image_t image_wifi;
extern const image_t image_earth;
extern const image_t image_error;
extern const image_t icon_cloudy;
extern const image_t icon_leizhenyu;
extern const image_t icon_moon;
extern const image_t icon_qing;
extern const image_t icon_wenduji;
extern const image_t icon_wifi;
extern const image_t icon_yintian;
extern const image_t icon_zhongxue;
extern const image_t icon_zhongyu;
extern const image_t icon_na;

#endif /* __IMAGE_H__ */
