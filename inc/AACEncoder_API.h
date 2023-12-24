/**
 * @file AACEncoder_API.h
 * @brief AAC Encoder API
 *
 * @defgroup aac_api AAC Encoder
 * @brief API and Structure for AAC encoder
 * @{
 */

/**
 * @example example_aac_encode.c
 */

#ifndef __AACENCODER_API_H__
#define __AACENCODER_API_H__

/**
 * @brief Create AAC encoder object
 * @return handler pointer: success, NULL: fail
 */
void *AACEncoder_API_context_Create();

/**
 * @brief AAC encoder process
 * @param[in] API pointer to the handler
 * @param[in] Input input data
 * @param[out] Output output data
 * @param[in] InputLen length of input data
 * @return 0: success, minus: fail
 */
int AACEncoder_API_Process(void *API,
		short *Input, char *Output, int InputLen);

/**
 * @brief Free AAC encoder object
 * @param[in] API pointer to the handler
 */
void AACEncoder_API_free(void *API);

/**
 * @brief Set AAC encoder parameters
 * @param[in] API pointer to the handler
 * @param[in] pParameters pointer to the parameters
 * @param[in] size fixed value 4
 * @param[in] IDs parameter id
 * @return 0: success, minus: fail
 */
int AACEncoder_API_Set(void *API,
		void *pParameters, int size, unsigned int IDs);

/**
 * @brief Get AAC encoder parameters
 * @param[in] API pointer to the handler
 * @param[out] pParameters pointer to the parameters
 * @param[in] size fixed value 4
 * @param[in] IDs parameter id
 * @return 0: success, minus: fail
 */
int AACEncoder_API_Get(void *API,
		void *pParameters, int size, unsigned int IDs);

/**
 * @brief Get block length for AAC encoder object
 * @param[in] API pointer to the handler
 * @return length: success, minus: fail
 */
int AACEncoder_API_GetBlockLen(void *API);

#endif /* __AACENCODER_API_H__ */
/*!@}*/
