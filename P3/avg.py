if False:
    for fileName in ["c032.txt","c064.txt","c128.txt","c256.txt","r032.txt","r064.txt","r128.txt","r256.txt"]:
        with open(fileName, "r") as file:
            deviceLine = True
            GPUtotal = 0
            CPUtotal = 0
            count = 0
            for line in file:
                if deviceLine:
                    assert line == "Using Device 0: GeForce GTX 1660 Ti\n"
                    deviceLine = False
                else:
                    (GPUtime, CPUtime, diff) = line.split(",")
                    assert int(diff) == 0
                    GPUtotal += float(GPUtime)
                    CPUtotal += float(CPUtime)
                    count += 1
                    deviceLine = True
            print(f"{fileName} - (GPU time: {GPUtotal/count}) (CPU time: {CPUtotal/count})")

if False:
    for fileName in ["c032_2.txt","c064_2.txt","c128_2.txt","c256_2.txt","r032_2.txt","r064_2.txt","r128_2.txt","r256_2.txt"]:
        with open(fileName, "r") as file:
            count = 0
            gpu1Total = 0
            gpu2Total = 0
            gpu3Total = 0
            cpu1Total = 0
            cpu2Total = 0
            cpu3Total = 0
            while True:
                device = file.readline()
                if device:
                    assert device == "Using Device 0: GeForce GTX 1660 Ti\n"
                    count += 1
                    gpu1Total += float(file.readline()[5:])
                    gpu2Total += float(file.readline()[5:])
                    gpu3Total += float(file.readline()[5:])
                    cpu1Total += float(file.readline()[5:])
                    cpu2Total += float(file.readline()[5:])
                    cpu3Total += float(file.readline()[5:])
                    diff = int(file.readline()[5:])
                    assert diff == 0
                else:
                    break
            print(f"{fileName}:")
            print(f"\tGPU: {gpu2Total/count:.10f} (+{(gpu1Total/count)+(gpu3Total/count):.10f})")
            print(f"\tCPU: {cpu2Total/count:.10f} (+{(cpu1Total/count)+(cpu3Total/count):.10f})")

if False:
    for fileName in ["c032_3.txt","c064_3.txt","c128_3.txt","c256_3.txt","r032_3.txt","r064_3.txt","r128_3.txt","r256_3.txt"]:
        with open(fileName, "r") as file:
            deviceLine = True
            CPUtotal = 0
            GPUcalc = 0
            GPUover = 0
            count = 0
            for line in file:
                if deviceLine:
                    deviceLine = False
                    assert line == "Using Device 0: GeForce GTX 1660 Ti\n"
                else:
                    deviceLine = True
                    (memcpy1, memcpy2, GPUtime, CPUtime, diff) = line.split(",")
                    assert int(diff) == 0
                    memcpy = float(memcpy1) + float(memcpy2)
                    CPUtotal += float(CPUtime)
                    GPUcalc += float(GPUtime) - memcpy
                    GPUover += memcpy
                    count += 1
            print(f"{fileName}:")
            print(f"\tGPU: {GPUcalc/count:.10f} (+{(GPUover/count):.10f})")
            print(f"\tCPU: {CPUtotal/count:.10f}")