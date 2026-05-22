import socket
import queue
import numpy as np
import sounddevice as sd

UDP_IP = "0.0.0.0"
UDP_PORT = 5005

SAMPLE_RATE = 16000
CHANNELS = 1
BLOCKSIZE = 1024  # 2048 samples por bloco, ajuste conforme necessário

VOLUME_GAIN = 1.0  # aumente/diminua aqui: 1.0, 2.0, 3.0, 5.0

audio_queue = queue.Queue(maxsize=100)

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))
sock.setblocking(False)


def audio_callback(outdata, frames, time, status):
    bytes_needed = frames * CHANNELS * 2
    data = bytearray()

    while len(data) < bytes_needed:
        try:
            data.extend(audio_queue.get_nowait())
        except queue.Empty:
            break

    if len(data) < bytes_needed:
        data.extend(b"\x00" * (bytes_needed - len(data)))

    samples = np.frombuffer(data[:bytes_needed], dtype=np.int16).astype(np.float32)

    samples *= VOLUME_GAIN

    samples = np.clip(samples, -32768, 32767).astype(np.int16)

    outdata[:] = samples.reshape(-1, CHANNELS)


stream = sd.OutputStream(
    samplerate=SAMPLE_RATE,
    channels=CHANNELS,
    dtype="int16",
    blocksize=BLOCKSIZE,
    callback=audio_callback
)

stream.start()

print(f"Escutando UDP na porta {UDP_PORT}...")
print(f"Ganho de volume: {VOLUME_GAIN}x")
print("Ctrl+C para parar")

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

            print(f"Pacote {len(data)} bytes de {addr}")

        except BlockingIOError:
            sd.sleep(1)

except KeyboardInterrupt:
    pass

stream.stop()
stream.close()
sock.close()