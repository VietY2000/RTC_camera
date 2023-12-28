#include <my_module.h>

void func_creat_new_folder(save_struct *file_save, time_t time) {

	struct tm *gtm_time = gmtime(&time);
	char gmt_string[64];
	strftime(gmt_string, sizeof(gmt_string), "%d-%m-%Y %H:%M:%S", gtm_time);

	strcpy(file_save->folder, file_save->path);

	uint16_t length = strlen(file_save->path);

	for (uint8_t p = 0; p < 10; p++)
	{
		file_save->folder[length + p] = gmt_string[p];
	}
	
	file_save->folder[length + 10] = '\0';

	if(mkdir(file_save->folder, 0777)){
		printf("error creat folder %s\n", file_save->folder);
	}
}

void func_set_file_name(save_struct *file_save, time_t time_start, uint32_t time_save) {
	
    struct tm *gmt_time_start = gmtime(&time_start);
	char time_start_string[64];
	strftime(time_start_string, sizeof(time_start_string), "%H%M%S_%d%m%Y", gmt_time_start);

	time_t time_after = time_start + time_save;
	struct tm *gmt_time_after = (struct tm *)malloc(sizeof(struct tm));
	gmtime_r(&time_after, gmt_time_after);

	strcpy(file_save->name, file_save->folder);

	uint16_t length = strlen(file_save->folder);

	file_save->name[length] = '/';

	for (uint8_t p = 0; p < 6; p++)
	{
		file_save->name[length + 1 + p] = time_start_string[p];
	}

	file_save->name[length + 7] = '_';

	// tomorrow
	if (gmt_time_start->tm_hour > gmt_time_after->tm_hour)
	{
		file_save->name[length + 8] = '\0';
		strcat(file_save->name, "235959");
	}	
	else
	{
		char time_after_string[25];
		strftime(time_after_string, sizeof(time_after_string), "%H%M%S_%d%m%Y", gmt_time_after);

		for (uint8_t p = 0; p < 6; ++p) {
			file_save->name[length + 8 + p] = time_after_string[p];
		}
	}
	free(gmt_time_after);

	file_save->name[length + 14] = '\0';

    switch (file_save->fm){
        case H264:
            strcat(file_save->name, ".h264\0");
            break;
        case ALAW:
            strcat(file_save->name, ".g711\0");
            break;
        case JPG:
            strcat(file_save->name, ".jpg\0");
            break;
        default:
            printf("error format\n");
            break;
    }
}

void func_save(save_struct *file_save, time_t *start, time_t now, uint32_t time_save)
{
	struct tm *gmt_now = gmtime(&now);

	uint32_t diff = difftime(now, *start);

	static bool change = 1;
	static bool new_file = 1;
	static bool new_file_jpg = 1;

	if ((gmt_now->tm_hour == 0 && gmt_now->tm_min == 0) && (gmt_now->tm_sec == 0 && change == 1)) {

		func_creat_new_folder(file_save, now);
		func_set_file_name(file_save, now, time_save);
		*start = now;
		change = 0;
		new_file = 1;
		new_file_jpg = 1;
		
	}
	else if (diff == time_save)
	{
		func_set_file_name(file_save, now, time_save);
		*start = now;
		change = 1;
		new_file = 1;
		new_file_jpg = 1;
		printf("<<<<<<<<<<<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>>>>>>>>>\n");
	}
	pthread_mutex_lock(&file_save->mt);

	if(file_save->fm == H264) {

		file_save->file = fopen(file_save->name, "a");
		if (!file_save->file) {
            printf("ERROR open file %s\n", file_save->name);
        }

		struct rts_av_buffer *buffer = NULL;

		rts_av_recv_block(file_save->channel, &buffer, 100);

		if(new_file){

			if(buffer->bytesused >= 10000){
				fwrite(buffer->vm_addr, 1, buffer->bytesused, file_save->file);
				new_file = 0;
			}
		}
		else{
			fwrite(buffer->vm_addr, 1, buffer->bytesused, file_save->file);
		}

		rts_av_put_buffer(buffer);	
		buffer = NULL;
		fclose(file_save->file);
	}
	else if (file_save->fm == JPG) { 

		if(new_file_jpg){

			file_save->file = fopen(file_save->name, "a");
			if (!file_save->file) {
            	printf("ERROR open file %s\n", file_save->name);
        	}

			struct rts_av_buffer *buffer = NULL;

			rts_av_recv_block(file_save->channel, &buffer, 50);

			fwrite(buffer->vm_addr, 1, buffer->bytesused, file_save->file);
			new_file_jpg = 0;

			rts_av_put_buffer(buffer);

			fclose(file_save->file);
			buffer = NULL;
		}	
	}
	else {
		file_save->file = fopen(file_save->name, "a");
		if (!file_save->file) {
            printf("ERROR open file %s\n", file_save->name);
        }

		struct rts_av_buffer *buffer = NULL;

		rts_av_recv_block(file_save->channel, &buffer, 100);

		fwrite(buffer->vm_addr, 1, buffer->bytesused, file_save->file);

		rts_av_put_buffer(buffer);
		buffer = NULL;
		fclose(file_save->file);
	}
	pthread_mutex_unlock(&file_save->mt);
}