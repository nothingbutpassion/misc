
__kernel __attribute__((vec_type_hint(double))) 
void sum(
	__global const double* input,
	__global double* partialSums,
	__local double* localSums)
{
	uint local_id = get_local_id(0);
	uint group_size = get_local_size(0);

	// Copy from global to local memory
	localSums[local_id] = input[get_global_id(0)];

	// Loop for computing localSums : divide WorkGroup into 2 parts
	for (uint stride = group_size / 2; stride>0; stride /= 2)
	{
		// Waiting for each 2x2 addition into given workgroup
		barrier(CLK_LOCAL_MEM_FENCE);

		// Add elements 2 by 2 between local_id and local_id + stride
		if (local_id < stride)
			localSums[local_id] += localSums[local_id + stride];
	}

	// Write result into partialSums[nWorkGroups]
	if (local_id == 0)
		partialSums[get_group_id(0)] = localSums[0];
}
