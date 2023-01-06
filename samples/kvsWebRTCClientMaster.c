#include "Samples.h"

extern PSampleConfiguration gSampleConfiguration;

// #define VERBOSE

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>

#include <pthread.h>

// pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
// pthread_mutex_lock( &mutex1 ); // 上鎖

// 子執行緒函數
// void *child(void *arg) {
//    my_data *data=(my_data *)arg; // 取得資料

//    int a = data->a;
//    int b = data->b;
//    int result = a + b; // 進行計算

//    data->result = result; // 將結果放進 data 中
//    pthread_exit(NULL);
// }

#define SVR_IP                          "127.0.0.1"
#define SVR_PORT                        8000
#define BUF_SIZE                        2048
// const int BUFFER_LEN = 2048;

int recv_len;
char socket_buffer[BUF_SIZE] = { 0 };

int StartSocketServer()
{
    struct sockaddr_in  server_addr;
    socklen_t           len;
    fd_set              active_fd_set;
    int                 sock_fd;
    int                 max_fd;
    int                 flag = 1;
    // char                socket_buffer[BUF_SIZE];
     
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SVR_IP);
    server_addr.sin_port = htons(SVR_PORT);
    len = sizeof(struct sockaddr_in);
 
    /* Create endpoint */
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket()");
        return -1;
    } else {
        printf("sock_fd=[%d]\n", sock_fd);
    }
 
    /* Set socket option */
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) < 0) {
        perror("setsockopt()");
        return -1;
    }
 
    /* Bind */
    if (bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind()");
        return -1;
    } else {
        printf("bind [%s:%u] success\n",
            inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));
    }
 
    /* Listen */
    if (listen(sock_fd, 128) == -1) {
        perror("listen()");
        return -1;
    }
 
    FD_ZERO(&active_fd_set);
    FD_SET(sock_fd, &active_fd_set);
    max_fd = sock_fd;
 
    while (1) {
        int             ret;
        struct timeval  tv;
        fd_set          read_fds;
 
        /* Set timeout */
        tv.tv_sec = 2;
        tv.tv_usec = 0;
 
        /* Copy fd set */
        read_fds = active_fd_set;
        ret = select(max_fd + 1, &read_fds, NULL, NULL, &tv);
        if (ret == -1) {
            perror("select()");
            return -1;
        } else if (ret == 0) {
            printf("select timeout\n");
            continue;
        } else {
            int i;
 
            /* Service all sockets */
            for (i = 0; i < FD_SETSIZE; i++) {
                if (FD_ISSET(i, &read_fds)) {
                    if (i == sock_fd) {
                        /* Connection request on original socket. */
                        struct sockaddr_in  client_addr;
                        int                 new_fd;
 
                        /* Accept */
                        new_fd = accept(sock_fd, (struct sockaddr *)&client_addr, &len);
                        if (new_fd == -1) {
                            perror("accept()");
                            return -1;
                        } else {
                            printf("Accpet client come from [%s:%u] by fd [%d]\n",
                                inet_ntoa(client_addr.sin_addr),
                                ntohs(client_addr.sin_port), new_fd);
 
                            /* Add to fd set */
                            FD_SET(new_fd, &active_fd_set);
                            if (new_fd > max_fd)
                                max_fd = new_fd;
                        }
                    } else {
                        /* Data arriving on an already-connected socket */
                        // int recv_len;
 
                        /* Receive */
                        memset(socket_buffer, 0, sizeof(socket_buffer));
                        recv_len = recv(i, socket_buffer, sizeof(socket_buffer), 0);
                        if (recv_len == -1) {
                            perror("recv()");
                            return -1;
                        } else if (recv_len == 0) {
                            // printf("Client disconnect\n");
                        } else {
                            // printf("Receive: len=[%d]", recv_len);
                            // printf("Receive: len=[%d] msg=[%s]\n", recv_len, socket_buffer);
 
                            /* Send (In fact we should determine when it can be written)*/
                            // send(i, buff, recv_len, 0);
                        }
 
                        /* Clean up */
                        // close(i);
                        // FD_CLR(i, &active_fd_set);
                    }
 
                } // end of if
            } //end of for
        } // end of if
    } // end of while

    for (int i = 0; i < FD_SETSIZE; i++) {
        close(i);
        FD_CLR(i, &active_fd_set);
    }
    return 0;
}


INT32 main(INT32 argc, CHAR* argv[])
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 frameSize;
    PSampleConfiguration pSampleConfiguration = NULL;
    SignalingClientMetrics signalingClientMetrics;
    PCHAR pChannelName;
    signalingClientMetrics.version = SIGNALING_CLIENT_METRICS_CURRENT_VERSION;

    pthread_t t;

    pthread_create(&t, NULL, StartSocketServer, NULL);//, (void*) &socket_buffer);

    SET_INSTRUMENTED_ALLOCATORS();

#ifndef _WIN32
    signal(SIGINT, sigintHandler);
#endif

    // do trickleIce by default
    printf("[KVS Master] Using trickleICE by default\n");

#ifdef IOT_CORE_ENABLE_CREDENTIALS
    CHK_ERR((pChannelName = getenv(IOT_CORE_THING_NAME)) != NULL, STATUS_INVALID_OPERATION, "AWS_IOT_CORE_THING_NAME must be set");
#else
    pChannelName = argc > 1 ? argv[1] : SAMPLE_CHANNEL_NAME;
#endif

    retStatus = createSampleConfiguration(pChannelName, SIGNALING_CHANNEL_ROLE_TYPE_MASTER, TRUE, TRUE, &pSampleConfiguration);
    if (retStatus != STATUS_SUCCESS) {
        printf("[KVS Master] createSampleConfiguration(): operation returned status code: 0x%08x \n", retStatus);
        goto CleanUp;
    }

    printf("[KVS Master] Created signaling channel %s\n", pChannelName);

    if (pSampleConfiguration->enableFileLogging) {
        retStatus =
            createFileLogger(FILE_LOGGING_BUFFER_SIZE, MAX_NUMBER_OF_LOG_FILES, (PCHAR) FILE_LOGGER_LOG_FILE_DIRECTORY_PATH, TRUE, TRUE, NULL);
        if (retStatus != STATUS_SUCCESS) {
            printf("[KVS Master] createFileLogger(): operation returned status code: 0x%08x \n", retStatus);
            pSampleConfiguration->enableFileLogging = FALSE;
        }
    }

    // Set the audio and video handlers
    // pSampleConfiguration->audioSource = sendAudioPackets;
    pSampleConfiguration->videoSource = sendVideoPackets;
    pSampleConfiguration->receiveAudioVideoSource = sampleReceiveVideoFrame;
    pSampleConfiguration->onDataChannel = onDataChannel;
    pSampleConfiguration->mediaType = SAMPLE_STREAMING_AUDIO_VIDEO;
    printf("[KVS Master] Finished setting audio and video handlers\n");

    // StartSocketServer();

    // Check if the samples are present

    // retStatus = readFrameFromDisk(NULL, &frameSize, "./h264SampleFrames/frame-0001.h264");
    // if (retStatus != STATUS_SUCCESS) {
    //     printf("[KVS Master] readFrameFromDisk(): operation returned status code: 0x%08x \n", retStatus);
    //     goto CleanUp;
    // }
    // printf("[KVS Master] Checked sample video frame availability....available\n");

    // retStatus = readFrameFromDisk(NULL, &frameSize, "./opusSampleFrames/sample-001.opus");
    // if (retStatus != STATUS_SUCCESS) {
    //     printf("[KVS Master] readFrameFromDisk(): operation returned status code: 0x%08x \n", retStatus);
    //     goto CleanUp;
    // }
    // printf("[KVS Master] Checked sample audio frame availability....available\n");

    // Initialize KVS WebRTC. This must be done before anything else, and must only be done once.
    retStatus = initKvsWebRtc();
    if (retStatus != STATUS_SUCCESS) {
        printf("[KVS Master] initKvsWebRtc(): operation returned status code: 0x%08x \n", retStatus);
        goto CleanUp;
    }
    printf("[KVS Master] KVS WebRTC initialization completed successfully\n");

    pSampleConfiguration->signalingClientCallbacks.messageReceivedFn = signalingMessageReceived;

    strcpy(pSampleConfiguration->clientInfo.clientId, SAMPLE_MASTER_CLIENT_ID);

    retStatus = createSignalingClientSync(&pSampleConfiguration->clientInfo, &pSampleConfiguration->channelInfo,
                                          &pSampleConfiguration->signalingClientCallbacks, pSampleConfiguration->pCredentialProvider,
                                          &pSampleConfiguration->signalingClientHandle);
    if (retStatus != STATUS_SUCCESS) {
        printf("[KVS Master] createSignalingClientSync(): operation returned status code: 0x%08x \n", retStatus);
        goto CleanUp;
    }
    printf("[KVS Master] Signaling client created successfully\n");

    // Enable the processing of the messages
    retStatus = signalingClientFetchSync(pSampleConfiguration->signalingClientHandle);
    if (retStatus != STATUS_SUCCESS) {
        printf("[KVS Master] signalingClientFetchSync(): operation returned status code: 0x%08x \n", retStatus);
        goto CleanUp;
    }

    retStatus = signalingClientConnectSync(pSampleConfiguration->signalingClientHandle);
    if (retStatus != STATUS_SUCCESS) {
        printf("[KVS Master] signalingClientConnectSync(): operation returned status code: 0x%08x \n", retStatus);
        goto CleanUp;
    }
    printf("[KVS Master] Signaling client connection to socket established\n");

    gSampleConfiguration = pSampleConfiguration;

    printf("[KVS Master] Channel %s set up done \n", pChannelName);

    // Checking for termination
    retStatus = sessionCleanupWait(pSampleConfiguration);
    if (retStatus != STATUS_SUCCESS) {
        printf("[KVS Master] sessionCleanupWait(): operation returned status code: 0x%08x \n", retStatus);
        goto CleanUp;
    }

    printf("[KVS Master] Streaming session terminated\n");

CleanUp:

    if (retStatus != STATUS_SUCCESS) {
        printf("[KVS Master] Terminated with status code 0x%08x\n", retStatus);
    }

    printf("[KVS Master] Cleaning up....\n");
    if (pSampleConfiguration != NULL) {
        // Kick of the termination sequence
        ATOMIC_STORE_BOOL(&pSampleConfiguration->appTerminateFlag, TRUE);

        if (IS_VALID_MUTEX_VALUE(pSampleConfiguration->sampleConfigurationObjLock)) {
            MUTEX_LOCK(pSampleConfiguration->sampleConfigurationObjLock);
        }

        // Cancel the media thread
        if (pSampleConfiguration->mediaThreadStarted) {
            DLOGD("Canceling media thread");
            THREAD_CANCEL(pSampleConfiguration->mediaSenderTid);
        }

        if (IS_VALID_MUTEX_VALUE(pSampleConfiguration->sampleConfigurationObjLock)) {
            MUTEX_UNLOCK(pSampleConfiguration->sampleConfigurationObjLock);
        }

        if (pSampleConfiguration->mediaSenderTid != INVALID_TID_VALUE) {
            THREAD_JOIN(pSampleConfiguration->mediaSenderTid, NULL);
        }

        if (pSampleConfiguration->enableFileLogging) {
            freeFileLogger();
        }
        retStatus = signalingClientGetMetrics(pSampleConfiguration->signalingClientHandle, &signalingClientMetrics);
        if (retStatus == STATUS_SUCCESS) {
            logSignalingClientStats(&signalingClientMetrics);
        } else {
            printf("[KVS Master] signalingClientGetMetrics() operation returned status code: 0x%08x\n", retStatus);
        }
        retStatus = freeSignalingClient(&pSampleConfiguration->signalingClientHandle);
        if (retStatus != STATUS_SUCCESS) {
            printf("[KVS Master] freeSignalingClient(): operation returned status code: 0x%08x", retStatus);
        }

        retStatus = freeSampleConfiguration(&pSampleConfiguration);
        if (retStatus != STATUS_SUCCESS) {
            printf("[KVS Master] freeSampleConfiguration(): operation returned status code: 0x%08x", retStatus);
        }
    }
    printf("[KVS Master] Cleanup done\n");

    RESET_INSTRUMENTED_ALLOCATORS();

    // https://www.gnu.org/software/libc/manual/html_node/Exit-Status.html
    // We can only return with 0 - 127. Some platforms treat exit code >= 128
    // to be a success code, which might give an unintended behaviour.
    // Some platforms also treat 1 or 0 differently, so it's better to use
    // EXIT_FAILURE and EXIT_SUCCESS macros for portability.
    return STATUS_FAILED(retStatus) ? EXIT_FAILURE : EXIT_SUCCESS;
}

STATUS readWebCam(PBYTE pFrame, PUINT32 pSize)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT64 size = 0;

    if (pSize == NULL) {
        printf("[KVS Master] readWebCam(): null operation returned status code: 0x%08x \n", STATUS_NULL_ARG);
        goto CleanUp;
    }

    size = *pSize;

    // Get the size and read into frame from global queue
    // retStatus = readFile(frameFilePath, TRUE, pFrame, &size);
    // pFrame = 
    // socket_buffer
    // int BUFFER_LEN = 2048;
    // size = 2048;

    if (pFrame != NULL) {
        // printf("recv_len: %d\n", recv_len);
        if (recv_len > 0) {
            // printf("copy");
            MEMCPY(pFrame, (PBYTE) socket_buffer, recv_len);
            size = recv_len;
        }
        // char socket_buffer[1024] = { 0 };
        // find the correct socket_buffer size
        // failed reason might be not suitable h264 format
    }

    // read from socket
    if (retStatus != STATUS_SUCCESS) {
        printf("[KVS Master] readFile(): wrong state operation returned status code: 0x%08x \n", retStatus);
        goto CleanUp;
    }

CleanUp:

    if (pSize != NULL) {
        *pSize = (UINT32) size;
    }

    return retStatus;
}

STATUS readFrameFromDisk(PBYTE pFrame, PUINT32 pSize, PCHAR frameFilePath)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT64 size = 0;

    if (pSize == NULL) {
        printf("[KVS Master] readFrameFromDisk(): operation returned status code: 0x%08x \n", STATUS_NULL_ARG);
        goto CleanUp;
    }

    size = *pSize;

    // Get the size and read into frame
    retStatus = readFile(frameFilePath, TRUE, pFrame, &size);
    // printf("%x", pFrame);
    if (retStatus != STATUS_SUCCESS) {
        printf("[KVS Master] readFile(): operation returned status code: 0x%08x \n", retStatus);
        goto CleanUp;
    }

CleanUp:

    if (pSize != NULL) {
        *pSize = (UINT32) size;
    }

    return retStatus;
}

PVOID sendVideoPackets(PVOID args)
{
    STATUS retStatus = STATUS_SUCCESS;
    PSampleConfiguration pSampleConfiguration = (PSampleConfiguration) args;
    RtcEncoderStats encoderStats;
    Frame frame;
    UINT32 fileIndex = 0, frameSize;
    CHAR filePath[MAX_PATH_LEN + 1];
    STATUS status;
    UINT32 i;
    UINT64 startTime, lastFrameTime, elapsed;
    MEMSET(&encoderStats, 0x00, SIZEOF(RtcEncoderStats));

    if (pSampleConfiguration == NULL) {
        printf("[KVS Master] sendVideoPackets(): operation returned status code: 0x%08x \n", STATUS_NULL_ARG);
        goto CleanUp;
    }

    frame.presentationTs = 0;
    startTime = GETTIME();
    lastFrameTime = startTime;

    while (!ATOMIC_LOAD_BOOL(&pSampleConfiguration->appTerminateFlag)) {
        fileIndex = fileIndex % NUMBER_OF_H264_FRAME_FILES + 1;
        snprintf(filePath, MAX_PATH_LEN, "./h264SampleFrames/frame-%04d.h264", fileIndex);

        // retStatus = readWebCam(NULL, &frameSize);//, filePath);
        // retStatus = readFrameFromDisk(NULL, &frameSize, filePath);
        // if (retStatus != STATUS_SUCCESS) {
        //     printf("[KVS Master] readFrameFromDisk(): operation returned status code: 0x%08x \n", retStatus);
        //     goto CleanUp;
        // }
        // printf("frame size: %d\n", frameSize);
        if (recv_len > 0) {
            frameSize = recv_len;
        }
        // Re-alloc if needed
        if (frameSize > pSampleConfiguration->videoBufferSize) {
            pSampleConfiguration->pVideoFrameBuffer = (PBYTE) MEMREALLOC(pSampleConfiguration->pVideoFrameBuffer, frameSize);
            if (pSampleConfiguration->pVideoFrameBuffer == NULL) {
                printf("[KVS Master] Video frame Buffer reallocation failed...%s (code %d)\n", strerror(errno), errno);
                printf("[KVS Master] MEMREALLOC(): operation returned status code: 0x%08x \n", STATUS_NOT_ENOUGH_MEMORY);
                goto CleanUp;
            }

            pSampleConfiguration->videoBufferSize = frameSize;
        }

        printf("frame size: %d\n", frameSize);
        frame.frameData = pSampleConfiguration->pVideoFrameBuffer;
        frame.size = frameSize;

        // retStatus = readFrameFromDisk(frame.frameData, &frameSize, filePath);
        retStatus = readWebCam(frame.frameData, &frameSize);
        if (retStatus != STATUS_SUCCESS) {
            // printf("[KVS Master] readFrameFromDisk(): operation returned status code: 0x%08x \n", retStatus);
            printf("[KVS Master] readWebCam(): operation returned status code: 0x%08x \n", retStatus);
            goto CleanUp;
        }

        // based on bitrate of samples/h264SampleFrames/frame-*
        encoderStats.width = 640;
        encoderStats.height = 480;
        // encoderStats.width = 1280;
        // encoderStats.height = 720;
        encoderStats.targetBitrate = 262000; // rewrite target bitrate
        frame.presentationTs += SAMPLE_VIDEO_FRAME_DURATION;

        MUTEX_LOCK(pSampleConfiguration->streamingSessionListReadLock);
        for (i = 0; i < pSampleConfiguration->streamingSessionCount; ++i) {
            status = writeFrame(pSampleConfiguration->sampleStreamingSessionList[i]->pVideoRtcRtpTransceiver, &frame);
            encoderStats.encodeTimeMsec = 4; //1// update encode time to an arbitrary number to demonstrate stats update
            updateEncoderStats(pSampleConfiguration->sampleStreamingSessionList[i]->pVideoRtcRtpTransceiver, &encoderStats);
            if (status != STATUS_SRTP_NOT_READY_YET) {
                if (status != STATUS_SUCCESS) {
#ifdef VERBOSE
                    printf("writeFrame() failed with 0x%08x\n", status);
#endif
                }
            }
        }
        MUTEX_UNLOCK(pSampleConfiguration->streamingSessionListReadLock);

        // Adjust sleep in the case the sleep itself and writeFrame take longer than expected. Since sleep makes sure that the thread
        // will be paused at least until the given amount, we can assume that there's no too early frame scenario.
        // Also, it's very unlikely to have a delay greater than SAMPLE_VIDEO_FRAME_DURATION, so the logic assumes that this is always
        // true for simplicity.
        elapsed = lastFrameTime - startTime;
        THREAD_SLEEP(SAMPLE_VIDEO_FRAME_DURATION - elapsed % SAMPLE_VIDEO_FRAME_DURATION);
        lastFrameTime = GETTIME();
    }

CleanUp:

    CHK_LOG_ERR(retStatus);

    return (PVOID) (ULONG_PTR) retStatus;
}

// PVOID sendAudioPackets(PVOID args)
// {
//     STATUS retStatus = STATUS_SUCCESS;
//     PSampleConfiguration pSampleConfiguration = (PSampleConfiguration) args;
//     Frame frame;
//     UINT32 fileIndex = 0, frameSize;
//     CHAR filePath[MAX_PATH_LEN + 1];
//     UINT32 i;
//     STATUS status;

//     if (pSampleConfiguration == NULL) {
//         printf("[KVS Master] sendAudioPackets(): operation returned status code: 0x%08x \n", STATUS_NULL_ARG);
//         goto CleanUp;
//     }

//     frame.presentationTs = 0;

//     while (!ATOMIC_LOAD_BOOL(&pSampleConfiguration->appTerminateFlag)) {
//         fileIndex = fileIndex % NUMBER_OF_OPUS_FRAME_FILES + 1;
//         snprintf(filePath, MAX_PATH_LEN, "./opusSampleFrames/sample-%03d.opus", fileIndex);

//         retStatus = readFrameFromDisk(NULL, &frameSize, filePath);
//         if (retStatus != STATUS_SUCCESS) {
//             printf("[KVS Master] readFrameFromDisk(): operation returned status code: 0x%08x \n", retStatus);
//             goto CleanUp;
//         }

//         // Re-alloc if needed
//         if (frameSize > pSampleConfiguration->audioBufferSize) {
//             pSampleConfiguration->pAudioFrameBuffer = (UINT8*) MEMREALLOC(pSampleConfiguration->pAudioFrameBuffer, frameSize);
//             if (pSampleConfiguration->pAudioFrameBuffer == NULL) {
//                 printf("[KVS Master] Audio frame Buffer reallocation failed...%s (code %d)\n", strerror(errno), errno);
//                 printf("[KVS Master] MEMREALLOC(): operation returned status code: 0x%08x \n", STATUS_NOT_ENOUGH_MEMORY);
//                 goto CleanUp;
//             }
//         }

//         frame.frameData = pSampleConfiguration->pAudioFrameBuffer;
//         frame.size = frameSize;

//         retStatus = readFrameFromDisk(frame.frameData, &frameSize, filePath);
//         if (retStatus != STATUS_SUCCESS) {
//             printf("[KVS Master] readFrameFromDisk(): operation returned status code: 0x%08x \n", retStatus);
//             goto CleanUp;
//         }

//         frame.presentationTs += SAMPLE_AUDIO_FRAME_DURATION;

//         MUTEX_LOCK(pSampleConfiguration->streamingSessionListReadLock);
//         for (i = 0; i < pSampleConfiguration->streamingSessionCount; ++i) {
//             status = writeFrame(pSampleConfiguration->sampleStreamingSessionList[i]->pAudioRtcRtpTransceiver, &frame);
//             if (status != STATUS_SRTP_NOT_READY_YET) {
//                 if (status != STATUS_SUCCESS) {
// #ifdef VERBOSE
//                     printf("writeFrame() failed with 0x%08x\n", status);
// #endif
//                 }
//             }
//         }
//         MUTEX_UNLOCK(pSampleConfiguration->streamingSessionListReadLock);
//         THREAD_SLEEP(SAMPLE_AUDIO_FRAME_DURATION);
//     }

// CleanUp:

//     return (PVOID) (ULONG_PTR) retStatus;
// }

PVOID sampleReceiveVideoFrame(PVOID args)
{
    STATUS retStatus = STATUS_SUCCESS;
    PSampleStreamingSession pSampleStreamingSession = (PSampleStreamingSession) args;
    if (pSampleStreamingSession == NULL) {
        printf("[KVS Master] sampleReceiveVideoFrame(): operation returned status code: 0x%08x \n", STATUS_NULL_ARG);
        goto CleanUp;
    }

    retStatus = transceiverOnFrame(pSampleStreamingSession->pVideoRtcRtpTransceiver, (UINT64) pSampleStreamingSession, sampleFrameHandler);
    if (retStatus != STATUS_SUCCESS) {
        printf("[KVS Master] transceiverOnFrame(): operation returned status code: 0x%08x \n", retStatus);
        goto CleanUp;
    }

CleanUp:

    return (PVOID) (ULONG_PTR) retStatus;
}
