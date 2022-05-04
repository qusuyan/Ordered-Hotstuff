Ordered Hotstuff
-----------
This project forks from the vanilla Hotstuff BFT protocol at https://github.com/hot-stuff/libhotstuff.git. We add ordering semantics to it so that a faulty leader cannot dictate the ordering of commands within each batch. The basic idea is that, during the prepare phase, when each node decides whether or not to accept a batch, they additionally send the order in which they see the requests. The leader collects these orders and broadcast them to the followers, so that the followers can run some deterministic algorithm to figure out the actual order in which the commands should be executed in. The idea coincides with the 3-stage approach proposed in https://eprint.iacr.org/2020/269.pdf, and this is just one implementation on Hotstuff. 

More implementation and evaluation details in https://github.com/qusuyan/Ordered-Hotstuff/blob/master/Ordered_HotStuff.pdf. 

Try the Current Version
=======================
::

    # install from the repo
    git clone https://github.com/hot-stuff/libhotstuff.git
    cd libhotstuff/
    git submodule update --init --recursive

    # ensure openssl and libevent are installed on your machine, more
    # specifically, you need:
    #
    # CMake >= 3.9 (cmake)
    # C++14 (g++)
    # libuv >= 1.10.0 (libuv1-dev)
    # openssl >= 1.1.0 (libssl-dev)
    #
    # on Ubuntu: sudo apt-get install libssl-dev libuv1-dev cmake make

    cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED=ON -DHOTSTUFF_PROTO_LOG=ON
    make

    # start 4 demo replicas with scripts/run_demo.sh
    # then, start the demo client with scripts/run_demo_client.sh


    # Fault tolerance:
    # Try to run the replicas as in run_demo.sh first and then run_demo_client.sh.
    # Use Ctrl-C to terminate the proposing replica (e.g. replica 0). Leader
    # rotation will be scheduled. Try to kill and run run_demo_client.sh again, new
    # commands should still get through (be replicated) once the new leader becomes
    # stable. Or try the following script:
    # scripts/faulty_leader_demo.sh

