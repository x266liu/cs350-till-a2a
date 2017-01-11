cd os161-1.99/kern/compile/ASST2
bmake clean
bmake depend
bmake 
bmake install
cd ../../
bmake 
bmake install
cd ~/cs350-os161/root
sys161 kernel