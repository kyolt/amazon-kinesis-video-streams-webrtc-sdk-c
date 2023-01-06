import av
import sys
import time
import socket
import cv2
FPS = 30
SLEEP_TIME = 0.04#1/FPS
HOST = '0.0.0.0'
PORT = 8000

socket_client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
socket_client.connect((HOST, PORT))

def main(rtsp):
	output = av.open('output_av.mp4', 'w')
	stream = output.add_stream('h264', '30')
	stream.bit_rate = 8000000

	# for i, img in enumerate(images):
	# 	frame = av.VideoFrame.from_ndarray(img, format='bgr24')
	# 	packet = stream.encode(frame)
	# 	output.mux(packet)

	if rtsp:
		c = av.open(rtsp)
	else:
		options = {'framerate': '30', 'pixel_format': 'yuyv422', 'video_size': '1280x720'}
		c = av.open(format='avfoundation', file='0', options=options)#rtsp)
	s = c.streams.video[0]
	# self.stream = self.container.streams.video[0]
	#     self.stream.thread_count = 2
	#     self.stream.thread_type = "AUTO"
	#     self.stream.codec_context.flags = "LOW_DELAY"
	#     self.stream.codec_context.skip_frame = settings.skip_frame
	#     self.video_demux = iter(self.container.demux(self.stream))
	v_demux = c.demux(s)

	for pkt in v_demux:
		# looks like we need to re-encode into h264
		# socket_client.send(pkt)
		frames = pkt.decode()
		# print("gogogo, ", len(frames))
		# socket_client.send(pkt.to_bytes())
		# socket_client.send(pkt)
# for frame in c.decode():
	# frame.to_image().save('frame-%04d.jpg' % frame.index)
		for frame in frames:
			# print("xxxxxxxxxxx")
			img = frame.to_ndarray(format="bgr24")
			packet = stream.encode(frame)
			if packet:
				# print(packet[0])
				socket_client.send(packet[0].to_bytes())
			cv2.imshow("test", img)
			time.sleep(SLEEP_TIME) # will have decode failed error if process too fast
			cv2.waitKey(1)
		# print("??????")



    # def decode_frame(self):
    #     video_packet = next(self.video_demux, None)
    #     if not video_packet:
    #         raise NoFrameException
    #     frame = video_packet.decode()
    #     if len(frame) != 0:
    #         return frame[0]
    #     else:
    #         raise NoFrameException

		# outdata = input('please input message: ')
		# print('send: ' + outdata)
		# socket_client.send(frame)
		
		# indata = s.recv(1024)
		# if len(indata) == 0: # connection closed
		# 	s.close()
		# 	print('server closed connection.')
		# 	break
		# print('recv: ' + indata.decode())


	cv2.destroyAllWindows()
		# send by socket
		# check byte size at first

if __name__ == "__main__":
	rtsp = "rtsp://rtsp01.lnv.ai/driving4370833.m4v"
	main(rtsp)
