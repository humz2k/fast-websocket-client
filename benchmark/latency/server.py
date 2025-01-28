import asyncio
import websockets
import time
import numpy as np
import pandas as pd
import tqdm


async def handler(websocket, path):
    client_name = await websocket.recv()
    print(client_name)

    await asyncio.sleep(1)

    rtts = []
    for msg_id in tqdm.trange(1000000):
        await websocket.send(str(msg_id))
        send_time = time.time()
        response = await websocket.recv()
        recv_time = time.time()
        assert int(response) == msg_id
        rtt = (recv_time - send_time) * 1e6
        rtts.append(rtt)
    rtts = np.array(rtts)
    print(
        f"MEAN: {np.mean(rtts)}, STD: {np.std(rtts)}, MAX: {np.max(rtts)}, MIN: {np.min(rtts)}"
    )
    df = pd.DataFrame()
    df["rtt"] = rtts
    df.to_csv(client_name + "_simple_latency.csv")

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
