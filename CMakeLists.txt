set(WITH_LKL "/home/oku/repos/lkl")

cmake_minimum_required(VERSION 3.0)
project(usbip-localbridge)

add_executable(localbridge
    localbridge.c)

if(WITH_LKL)
    add_definitions(
        -pthread
        -D_FILE_OFFSET_BITS=64)
    set(lkl_host_srcs_base
        fs.c
        iomem.c
        net.c
        jmp_buf.c
        posix-host.c
        utils.c
        virtio_net.c
        virtio_net_fd.c
        virtio_net_tap.c
        virtio_net_raw.c
        virtio_net_pipe.c
        virtio_net_macvtap.c
        virtio_blk.c
        virtio.c
        dbg.c
        dbg_handler.c
        ../../perf/pmu-events/jsmn.c
        config.c
        )
    set(lkl_host_srcs)
    foreach(e ${lkl_host_srcs_base})
        list(APPEND lkl_host_srcs ${WITH_LKL}/tools/lkl/lib/${e})
    endforeach()
    include_directories(
        ${WITH_LKL}/tools/lkl/include
        )
    add_executable(lklbridge
        lklbridge.c
        ${lkl_host_srcs})
    target_link_libraries(lklbridge
        ${WITH_LKL}/lkl.o
        rt
        pthread)
endif()
