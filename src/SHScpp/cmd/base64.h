/*
 * base64.h
 *
 *  Created on: Jan 22, 2018
 *      Author: pupu
 */

#ifndef SHSCPP_CMD_BASE64_H_
#define SHSCPP_CMD_BASE64_H_

#ifdef __cplusplus
extern "C" {
#endif

char *base64_encode(const unsigned char *data,
                    size_t input_length,
                    size_t *output_length);

/**
 * the provided encoded_data buffer should with a buffer at least 4 * ((input_length + 2) / 3)
 */
size_t base64_encode2(const unsigned char *data,
                    size_t input_length,
					unsigned char *encoded_data);


unsigned char *base64_decode(const char *data,
                             size_t input_length,
                             size_t *output_length);


#ifdef __cplusplus
}
#endif

#endif /* SHSCPP_CMD_BASE64_H_ */
