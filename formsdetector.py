import Image
import select
import time
import v4l2capture

video = v4l2capture.Video_device("/dev/video0")

size_x, size_y = video.set_format(500, 375)

video.create_buffers(1)

video.start()

time.sleep(1)

video.queue_all_buffers()

select.select((video,), (), ())

image_data = video.read()
video.close()
image = Image.fromstring("RGB", (size_x, size_y), image_data)

print image.histogram()
