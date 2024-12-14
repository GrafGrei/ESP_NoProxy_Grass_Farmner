# -*- coding: utf-8 -*-
# @Author   :Solana0x
# @File     :no_proxy.py
# @Software :PyCharm
import asyncio
import base64
import random
import ssl
import json
import time
import uuid

import requests
import traceback

import websockets
from loguru import logger



def header_to_lower(header_dict):
    out_header_dict = {}
    for field, value in header_dict.items():
        out_header_dict[field.lower()] = value
    return  out_header_dict

async def connect_to_wss(user_id):
    device_id = str(uuid.uuid4()) #"ebb49154-e4c4-5270-a7f4-8f7973a6ab49"   #"0578039e-8fbd-49ac-9ddd-8cac867d0aa0" "ebb49154-e4c4-5270-a7f4-8f7973a6ab49"
    logger.info(device_id)
    while True:
        try:
            await asyncio.sleep(random.randint(1, 10) / 10)
            custom_headers = {
                "user-agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/126.0.0.0 Safari/537.36 Edg/126.0.0.0"
            }
            ssl_context = ssl.create_default_context()
            ssl_context.check_hostname = False
            ssl_context.verify_mode = ssl.CERT_NONE
            urilist = ["wss://proxy2.wynd.network:4444/", "wss://proxy2.wynd.network:4650/"]
            uri = random.choice(urilist)
            server_hostname = "proxy2.wynd.network"
            async with websockets.connect(uri, ssl=ssl_context, extra_headers=custom_headers,
                                          server_hostname=server_hostname) as websocket:
                # async def send_ping():
                #     while True:
                #         send_message = json.dumps(
                #             {"id": str(uuid.uuid4()), "version": "1.0.0", "action": "PING", "data": {}})
                #         logger.debug(send_message)
                #         await websocket.send(send_message)
                #         await asyncio.sleep(120)
                #
                # await asyncio.sleep(1)
                # asyncio.create_task(send_ping())

                while True:
                    response = await websocket.recv()
                    message = json.loads(response)
                    logger.info(message)
                    if message.get("action") == "AUTH":
                        auth_response = {
                            "id": message["id"],
                            "origin_action": "AUTH",
                            "result": {
                                "browser_id": device_id,
                                "device_type": "desktop",
                                "timestamp": int(time.time()),
                                "user_agent": custom_headers['user-agent'],
                                "user_id": user_id,
                                "version": "4.30.0"
                                # "device_type": "extension",
                                # "version": "4.26.2",
                                # "extension_id":"lkbnfiajjmbhnfledhphioinpickokdi"
                            }
                        }
                        logger.debug(auth_response)
                        await websocket.send(json.dumps(auth_response))
                        await asyncio.sleep(1)
                        # logger.debug("{\"jwt\":{\"action\":\"getCurrentVersion\"},\"action\":\"VALIDATE_JWT\"}")
                        # await websocket.send("{\"jwt\":{\"action\":\"getCurrentVersion\"},\"action\":\"VALIDATE_JWT\"}")


                    elif message.get("action") == "PONG":
                        pong_response = {"id": message["id"], "origin_action": "PONG"}
                        logger.debug(pong_response)
                        await websocket.send(json.dumps(pong_response))
                    elif message.get("action") == "HTTP_REQUEST":
                        data = message['data']
                        url = data['url']
                        method = data['method']
                        headers = data['headers']
                        authenticated = data['authenticated']
                        req = requests.request(method, url, headers=headers, auth=authenticated)
                        headers_req = req.headers
                        print(base64.b64encode(req.content).decode())
                        print(req.content)
                        response = {
                            "id": message["id"],
                            "origin_action": "HTTP_REQUEST",
                            "result": {
                                "url": url,
                                "status": req.status_code,
                                "status_text": "OK",
                                "headers": header_to_lower(dict(headers_req)),
                                "body": base64.b64encode(req.content).decode(),
                            }
                        }
                        logger.debug(response)
                        logger.debug(json.dumps(response))
                        await websocket.send(json.dumps(response))


        except Exception as e:
            traceback.print_exc()


async def main():
    # TODO Modify user_id
    _user_id = ''

    #THIS BOT DONTS TAKE ANY PROXIES SO IT WILL TAKE YOUR DEVICE AS MAIN PROXIES !!
    await connect_to_wss(_user_id)


if __name__ == '__main__':
   # # Run the main function
    asyncio.run(main())
