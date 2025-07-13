﻿using Nefarius.Utilities.Bluetooth;

namespace BTControl {
    class Program
    {           
        static void Main(string[] args) {
            if (args.Length == 0) {
                return;
            }

            using var radio = new HostRadio();

            try { 
                radio.DisconnectRemoteDevice(args[0]);
                Console.WriteLine("Device disabled: " + args[0]);
            }
            catch {
                Console.WriteLine("Failed to disable device: " + args[0]);
            }
        }
    }
}