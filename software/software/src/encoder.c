#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

typedef struct {
    FILE* pipe;
    int width;
    int height;
    int fps;
    int frame_count;
    
    /* Frame drop logic */
    int frame_drop_threshold; /* Frames behind before we start dropping */
    double last_write_time;
    double expected_frame_time;
    char last_error[256];
} Encoder;

static double get_time(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

int encoder_is_available(void) {
    return access("/usr/bin/ffmpeg", X_OK) == 0;
}

Encoder* encoder_create(const char* filename, int width, int height, int fps) {
    if (!filename || width <= 0 || height <= 0 || fps <= 0) return NULL;
    if (!encoder_is_available()) {
        fprintf(stderr, "ENCODER ERROR: ffmpeg not found in PATH\n");
        return NULL;
    }
    
    Encoder* enc = (Encoder*)calloc(1, sizeof(Encoder));
    if (!enc) return NULL;
    
    enc->width = width;
    enc->height = height;
    enc->fps = fps;
    enc->frame_drop_threshold = 10; /* Drop if > 10 frames behind */
    enc->expected_frame_time = 1.0 / (double)fps;
    enc->last_write_time = get_time();
    
    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
        "ffmpeg -y -f rawvideo -pix_fmt rgb24 -s %dx%d -r %d -i pipe:0 "
        "-c:v libx264 -pix_fmt yuv420p -preset fast \"%s\" 2>/dev/null",
        width, height, fps, filename);
    
    enc->pipe = popen(cmd, "w");
    if (!enc->pipe) {
        snprintf(enc->last_error, sizeof(enc->last_error), "Failed to open ffmpeg pipe");
        free(enc);
        return NULL;
    }
    
    return enc;
}

void encoder_destroy(Encoder* enc) {
    if (!enc) return;
    if (enc->pipe) {
        pclose(enc->pipe);
        enc->pipe = NULL;
    }
    free(enc);
}

void encoder_set_frame_drop_threshold(Encoder* enc, int threshold) {
    if (enc) enc->frame_drop_threshold = threshold;
}

const char* encoder_last_error(Encoder* enc) {
    if (!enc) return "No encoder";
    return enc->last_error;
}

void encoder_write_frame(Encoder* enc, const uint8_t* rgb_data) {
    if (!enc || !enc->pipe || !rgb_data) return;
    
    double now = get_time();
    double elapsed = now - enc->last_write_time;
    
    /* If we are falling behind significantly, skip frames */
    /* Check if elapsed time is much larger than expected frame time */
    if (enc->frame_drop_threshold > 0) {
        double frames_behind = (elapsed / enc->expected_frame_time) - (double)enc->frame_count;
        /* Simplified logic: if wall clock is way ahead of frame count logic, skip */
        /* Better logic: check if time since last write is > N frames worth */
        if (elapsed > enc->frame_drop_threshold * enc->expected_frame_time) {
            /* Too slow, skip this frame to catch up */
            enc->last_write_time = now; 
            snprintf(enc->last_error, sizeof(enc->last_error), "Frame dropped due to latency");
            return;
        }
    }

    size_t frame_size = enc->width * enc->height * 3;
    size_t written = fwrite(rgb_data, 1, frame_size, enc->pipe);
    
    if (written != frame_size) {
        snprintf(enc->last_error, sizeof(enc->last_error), "Failed to write frame (wrote %zu/%zu)", written, frame_size);
    } else {
        enc->last_error[0] = '\0';
        enc->frame_count++;
        enc->last_write_time = now;
    }
}
