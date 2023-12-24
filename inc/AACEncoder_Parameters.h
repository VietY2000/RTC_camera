#ifndef _AACENCODER_PARAMETERS_
#define _AACENCODER_PARAMETERS_

/**
 * @addtogroup aac_api
 * @{
 */

/**
 * @brief AAC encoder parameter-0, ID = 0
 */
typedef struct {
	int m_SampleRate; /**< sample rate */
	int m_BitRate; /**< bit rate */
} AacEncoder_Para0;

/**
 * @brief AAC encoder parameter-1, ID = 1
 */
typedef struct {
	int m_Channels; /**< channels */
} AacEncoder_Para1;
/*!@}*/

#endif
