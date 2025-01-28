import asyncio
import websockets
import time
import numpy as np
import pandas as pd
import tqdm


async def send_messages(websocket, n_messages, send_times, delay):
    for msg_id in tqdm.trange(n_messages, desc="Sending"):
        await websocket.send(str(msg_id))
        send_times[msg_id] = time.time()
        # Simulate some delay between sends
        await asyncio.sleep(delay)


async def receive_messages(websocket, n_messages, recv_times):
    for _ in range(n_messages):
        response = await websocket.recv()
        msg_id = int(response)
        recv_times[msg_id] = time.time()


async def handler(websocket, path):
    send_times = {}
    recv_times = {}
    client_name = await websocket.recv()
    print(client_name)

    await asyncio.sleep(1)

    n_messages = 10000
    msg_per_second = 1000

    # Create tasks for sending and receiving
    send_task = asyncio.create_task(
        send_messages(websocket, n_messages, send_times, 1 / msg_per_second)
    )
    recv_task = asyncio.create_task(receive_messages(websocket, n_messages, recv_times))

    # Wait until both sending and receiving finish
    await asyncio.gather(send_task, recv_task)

    rtts = []
    for msg_id in range(n_messages):
        rtt = (recv_times[msg_id] - send_times[msg_id]) * 1e6
        rtts.append(rtt)
    rtts = np.array(rtts)
    print(
        f"MEAN: {np.mean(rtts)}, STD: {np.std(rtts)}, MAX: {np.max(rtts)}, MIN: {np.min(rtts)}"
    )
    df = pd.DataFrame()
    df["rtt"] = rtts
    df.to_csv(client_name + "_many_latency.csv")

    await websocket.close()


async def main():
    async with websockets.serve(handler, "localhost", 8765):
        print("WebSocket server started on ws://localhost:8765")
        await asyncio.Future()


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nReceived KeyboardInterrupt, shutting down gracefully.")
