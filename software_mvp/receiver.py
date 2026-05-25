import socket
import queue
import numpy as np
import sounddevice as sd

UDP_IP = "0.0.0.0"
UDP_PORT = 5005

SAMPLE_RATE = 16000
CHANNELS = 1
BLOCKSIZE = 512

PREBUFFER_PACKETS = 6

audio_queue = queue.Queue(maxsize=200)
started = False

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))
sock.setblocking(False)


def audio_callback(outdata, frames, time, status):
    global started

    bytes_needed = frames * CHANNELS * 2
    data = bytearray()

    if not started:
        if audio_queue.qsize() < PREBUFFER_PACKETS:
            outdata[:] = np.zeros((frames, CHANNELS), dtype=np.int16)
            return
        started = True

    while len(data) < bytes_needed:
        try:
            data.extend(audio_queue.get_nowait())
        except queue.Empty:
            break

    if len(data) < bytes_needed:
        # mantém silêncio, mas agora deve acontecer menos
        data.extend(b"\x00" * (bytes_needed - len(data)))

    samples = np.frombuffer(data[:bytes_needed], dtype=np.int16)
    outdata[:] = samples.reshape(-1, CHANNELS)


stream = sd.OutputStream(
    samplerate=SAMPLE_RATE,
    channels=CHANNELS,
    dtype="int16",
    blocksize=BLOCKSIZE,
    callback=audio_callback
)

stream.start()

print("Recebendo UDP...")

try:
    while True:
        try:
            data, addr = sock.recvfrom(4096)

            if len(data) % 2 != 0:
                data = data[:-1]

            try:
                audio_queue.put_nowait(data)
            except queue.Full:
                audio_queue.get_nowait()
                audio_queue.put_nowait(data)

            print("fila:", audio_queue.qsize(), "pacote:", len(data))

        except BlockingIOError:
            sd.sleep(1)

except KeyboardInterrupt:
    pass

stream.stop()
stream.close()
sock.close()