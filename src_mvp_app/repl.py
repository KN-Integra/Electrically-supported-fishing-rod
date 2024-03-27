# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2023 Sebastian Soczawa
#

import asyncio
import os
from bluetoothclient import BluetoothClient

script_dir = os.path.dirname(os.path.abspath(__file__))
HELP_PATH = os.path.join(script_dir, "help.txt")


async def start_repl():
    HELP = ""
    try:
        with open(HELP_PATH, "r") as file:
            HELP = file.read()
    except FileNotFoundError:
        print("cannot locate help.txt")

    client = BluetoothClient()

    running = True

    print("Welcome to the WWE repl program!\nType 'help' for more information")
    try:
        while running:
            try:
                greeter = (
                    "|| WWE : connected || >> "
                    if client.check_connection()
                    else "|| WWE || >> "
                )
                _in = input(greeter)
                cmd = _in.split(" ")
                if cmd[0] == "scan":
                    scan_result = await client.cmd_scan()
                    for device in scan_result:
                        print(device.address, device.name)

                elif cmd[0] == "connect":
                    if cmd[1] == "auto":
                        await client.cmd_autoconnect()

                    elif cmd[1] == "check":

                        print(client.check_connection())
                    else:
                        await client.cmd_connect(cmd[1])

                elif cmd[0] == "help":
                    print(HELP)

                elif cmd[0] == "exit":
                    print("Closing repl.")
                    if client.check_connection():
                        await client.cmd_disconnect()
                    return

                elif client.check_connection():
                    if cmd[0] == "init":
                        await client.cmd_init()

                    elif cmd[0] == "speed":
                        if cmd[1] == "get":
                            speed = await client.cmd_speed_get()
                            print("Current target speed: " + str(speed))

                        elif cmd[1] == "set":
                            await client.cmd_speed_set(int(cmd[2]))

                    elif cmd[0] == "boot":
                        await client.cmd_boot()

                    elif cmd[0] == "driver":
                        driver_version = await client.cmd_driver()
                        print("Driver version: " + str(driver_version))

                    elif cmd[0] == "disconnect":
                        await client.cmd_disconnect()

                else:
                    print("Invalid command or state. Please type: 'help' for help")
            except Exception as e:
                print(f"Error during input: {e}")

    except KeyboardInterrupt as e:
        print("\nClosing repl.")
        if client.check_connection():
            await client.cmd_disconnect()
        running = False
        return


if __name__ == "__main__":
    try:
        asyncio.run(start_repl())
    except KeyboardInterrupt as e:
        print("Please exit the program using the exit command next time")
