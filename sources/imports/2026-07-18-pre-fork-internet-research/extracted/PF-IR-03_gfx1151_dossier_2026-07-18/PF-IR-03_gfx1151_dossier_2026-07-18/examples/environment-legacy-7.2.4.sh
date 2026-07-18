# Source only in a dedicated control shell.
export ROCM_PATH=/opt/rocm
export PATH="$ROCM_PATH/bin:$PATH"
export LD_LIBRARY_PATH="$ROCM_PATH/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
unset HIP_PATH HIP_DEVICE_LIB_PATH ROCM_HOME
