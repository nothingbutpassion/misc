import os
import sys
import signal
import time
import numpy as np
import matplotlib.pyplot as plt

interrupted = False

def signal_handler(signum, frame):
    global interrupted
    interrupted = True

def get_pid(app_name):
    cmd = 'adb shell "ps -ef | grep -v grep | grep %s"' % app_name
    output = os.popen(cmd)
    output = output.readline()
    fs = output.split()
    if len(fs) > 1:
        return fs[1]
    return ""

def get_meminfo(pid):
    cmd = 'adb shell "dumpsys meminfo %s"' % pid
    output = os.popen(cmd)
    lines = output.readlines()
    result = {"java_heaps": None, "native_heaps": None, "total": None}
    for line in lines:
        fs = line.strip().split()
        if len(fs) > 2:
            if fs[0] == "Java" and fs[1] == "Heap:":
                result["java_heaps"] = int(fs[2])
            if fs[0] == "Native" and fs[1] == "Heap:":
                result["native_heaps"] = int(fs[2])
            if fs[0] == "TOTAL":
                result["total"] = int(fs[1])
    return result

def show_meminfo(meminfo, save_path):
    java_heaps = np.float32([m["java_heaps"] for m in meminfo])
    native_heaps = np.float32([m["native_heaps"] for m in meminfo])
    total = np.float32([m["total"] for m in meminfo])
    plt.plot(java_heaps, "b", label="Java Heaps")
    plt.plot(native_heaps, "g", label="Native Heaps")
    plt.plot(total, "r", label="Total")
    plt.title('Memory Usage')
    plt.xlabel('Time (s)')
    plt.ylabel('Memory (KB)')
    plt.legend()
    if save_path != None:
        plt.savefig(save_path)
    plt.show()

def get_cpuinfo(app_name):
    cmd = 'adb shell "top -n 1 -d 1 | grep -v grep | grep %s"' % app_name
    output = os.popen(cmd)
    output = output.readline().strip()
    fs = output.split()
    if len(fs) > 8:
        return fs[8]
    return ""

def show_cpuinfo(cpuinfo, save_path):
    percent = np.float32([c["percent"] for c in cpuinfo])
    plt.plot(percent, "r", label="%CPU")
    plt.title('CPU Usage')
    plt.xlabel('Time (s)')
    plt.ylabel('%CPU')
    if save_path != None:
        plt.savefig(save_path)
    plt.show()

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: <this-script> <app-name>")
        sys.exit(-1)
    app_name = sys.argv[1]
    signal.signal(signal.SIGINT, signal_handler)
    meminfo = []
    cpuinfo = []
    while not interrupted:
        pid = get_pid(app_name)
        if pid != "":
            m = get_meminfo(pid)
            if m["java_heaps"] != None and m["native_heaps"] != None and m["total"] != None:
                meminfo.append(m)
                print(m)
        cpu_percent = get_cpuinfo(app_name)
        if cpu_percent != "":
            p = {"percent": float(cpu_percent)}
            print(p)
            cpuinfo.append(p)
    
    time.sleep(1)
    response = input("Save figure to png? [y/n]: ")
    meminfo_png_file = None
    cpuinfo_png_file = None
    if response.lower() == "y":
        meminfo_png_file = time.strftime("%Y-%m-%d-%H-%M-%S_mem.png")
        cpuinfo_png_file = time.strftime("%Y-%m-%d-%H-%M-%S_cpu.png")
    if len(meminfo) > 0:
        show_meminfo(meminfo, meminfo_png_file)
    if len(cpuinfo) > 0:
        show_cpuinfo(cpuinfo, cpuinfo_png_file)