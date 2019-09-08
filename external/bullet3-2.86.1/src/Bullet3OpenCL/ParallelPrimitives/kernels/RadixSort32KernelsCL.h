//this file is autogenerated using stringify.bat (premake --stringify) in the build folder of this project
static const char* radixSort32KernelsCL= \
"/*\n"
"Bullet Continuous Collision Detection and Physics Library\n"
"Copyright (c) 2011 Advanced Micro Devices, Inc.  http://bulletphysics.org\n"
"This software is provided 'as-is', without any express or implied warranty.\n"
"In no event will the authors be held liable for any damages arising from the use of this software.\n"
"Permission is granted to anyone to use this software for any purpose, \n"
"including commercial applications, and to alter it and redistribute it freely, \n"
"subject to the following restrictions:\n"
"1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.\n"
"2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.\n"
"3. This notice may not be removed or altered from any source distribution.\n"
"*/\n"
"//Author Takahiro Harada\n"
"//#pragma OPENCL EXTENSION cl_amd_printf : enable\n"
"#pragma OPENCL EXTENSION cl_khr_local_int32_base_atomics : enable\n"
"#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable\n"
"typedef unsigned int u32;\n"
"#define GET_GROUP_IDX get_group_id(0)\n"
"#define GET_LOCAL_IDX get_local_id(0)\n"
"#define GET_GLOBAL_IDX get_global_id(0)\n"
"#define GET_GROUP_SIZE get_local_size(0)\n"
"#define GROUP_LDS_BARRIER barrier(CLK_LOCAL_MEM_FENCE)\n"
"#define GROUP_MEM_FENCE mem_fence(CLK_LOCAL_MEM_FENCE)\n"
"#define AtomInc(x) atom_inc(&(x))\n"
"#define AtomInc1(x, out) out = atom_inc(&(x))\n"
"#define AtomAdd(x, value) atom_add(&(x), value)\n"
"#define SELECT_UINT4( b, a, condition ) select( b,a,condition )\n"
"#define make_uint4 (uint4)\n"
"#define make_uint2 (uint2)\n"
"#define make_int2 (int2)\n"
"#define WG_SIZE 64\n"
"#define ELEMENTS_PER_WORK_ITEM (256/WG_SIZE)\n"
"#define BITS_PER_PASS 4\n"
"#define NUM_BUCKET (1<<BITS_PER_PASS)\n"
"typedef uchar u8;\n"
"//	this isn't optimization for VLIW. But just reducing writes. \n"
"#define USE_2LEVEL_REDUCE 1\n"
"//#define CHECK_BOUNDARY 1\n"
"//#define NV_GPU 1\n"
"//	Cypress\n"
"#define nPerWI 16\n"
"//	Cayman\n"
"//#define nPerWI 20\n"
"#define m_n x\n"
"#define m_nWGs y\n"
"#define m_startBit z\n"
"#define m_nBlocksPerWG w\n"
"/*\n"
"typedef struct\n"
"{\n"
"	int m_n;\n"
"	int m_nWGs;\n"
"	int m_startBit;\n"
"	int m_nBlocksPerWG;\n"
"} ConstBuffer;\n"
"*/\n"
"typedef struct\n"
"{\n"
"	unsigned int m_key;\n"
"	unsigned int m_value;\n"
"} SortDataCL;\n"
"uint prefixScanVectorEx( uint4* data )\n"
"{\n"
"	u32 sum = 0;\n"
"	u32 tmp = data[0].x;\n"
"	data[0].x = sum;\n"
"	sum += tmp;\n"
"	tmp = data[0].y;\n"
"	data[0].y = sum;\n"
"	sum += tmp;\n"
"	tmp = data[0].z;\n"
"	data[0].z = sum;\n"
"	sum += tmp;\n"
"	tmp = data[0].w;\n"
"	data[0].w = sum;\n"
"	sum += tmp;\n"
"	return sum;\n"
"}\n"
"u32 localPrefixSum( u32 pData, uint lIdx, uint* totalSum, __local u32* sorterSharedMemory, int wgSize /*64 or 128*/ )\n"
"{\n"
"	{	//	Set data\n"
"		sorterSharedMemory[lIdx] = 0;\n"
"		sorterSharedMemory[lIdx+wgSize] = pData;\n"
"	}\n"
"	GROUP_LDS_BARRIER;\n"
"	{	//	Prefix sum\n"
"		int idx = 2*lIdx + (wgSize+1);\n"
"#if defined(USE_2LEVEL_REDUCE)\n"
"		if( lIdx < 64 )\n"
"		{\n"
"			u32 u0, u1, u2;\n"
"			u0 = sorterSharedMemory[idx-3];\n"
"			u1 = sorterSharedMemory[idx-2];\n"
"			u2 = sorterSharedMemory[idx-1];\n"
"			AtomAdd( sorterSharedMemory[idx], u0+u1+u2 );			\n"
"			GROUP_MEM_FENCE;\n"
"			u0 = sorterSharedMemory[idx-12];\n"
"			u1 = sorterSharedMemory[idx-8];\n"
"			u2 = sorterSharedMemory[idx-4];\n"
"			AtomAdd( sorterSharedMemory[idx], u0+u1+u2 );			\n"
"			GROUP_MEM_FENCE;\n"
"			u0 = sorterSharedMemory[idx-48];\n"
"			u1 = sorterSharedMemory[idx-32];\n"
"			u2 = sorterSharedMemory[idx-16];\n"
"			AtomAdd( sorterSharedMemory[idx], u0+u1+u2 );			\n"
"			GROUP_MEM_FENCE;\n"
"			if( wgSize > 64 )\n"
"			{\n"
"				sorterSharedMemory[idx] += sorterSharedMemory[idx-64];\n"
"				GROUP_MEM_FENCE;\n"
"			}\n"
"			sorterSharedMemory[idx-1] += sorterSharedMemory[idx-2];\n"
"			GROUP_MEM_FENCE;\n"
"		}\n"
"#else\n"
"		if( lIdx < 64 )\n"
"		{\n"
"			sorterSharedMemory[idx] += sorterSharedMemory[idx-1];\n"
"			GROUP_MEM_FENCE;\n"
"			sorterSharedMemory[idx] += sorterSharedMemory[idx-2];			\n"
"			GROUP_MEM_FENCE;\n"
"			sorterSharedMemory[idx] += sorterSharedMemory[idx-4];\n"
"			GROUP_MEM_FENCE;\n"
"			sorterSharedMemory[idx] += sorterSharedMemory[idx-8];\n"
"			GROUP_MEM_FENCE;\n"
"			sorterSharedMemory[idx] += sorterSharedMemory[idx-16];\n"
"			GROUP_MEM_FENCE;\n"
"			sorterSharedMemory[idx] += sorterSharedMemory[idx-32];\n"
"			GROUP_MEM_FENCE;\n"
"			if( wgSize > 64 )\n"
"			{\n"
"				sorterSharedMemory[idx] += sorterSharedMemory[idx-64];\n"
"				GROUP_MEM_FENCE;\n"
"			}\n"
"			sorterSharedMemory[idx-1] += sorterSharedMemory[idx-2];\n"
"			GROUP_MEM_FENCE;\n"
"		}\n"
"#endif\n"
"	}\n"
"	GROUP_LDS_BARRIER;\n"
"	*totalSum = sorterSharedMemory[wgSize*2-1];\n"
"	u32 addValue = sorterSharedMemory[lIdx+wgSize-1];\n"
"	return addValue;\n"
"}\n"
"//__attribute__((reqd_work_group_size(128,1,1)))\n"
"uint4 localPrefixSum128V( uint4 pData, uint lIdx, uint* totalSum, __local u32* sorterSharedMemory )\n"
"{\n"
"	u32 s4 = prefixScanVectorEx( &pData );\n"
"	u32 rank = localPrefixSum( s4, lIdx, totalSum, sorterSharedMemory, 128 );\n"
"	return pData + make_uint4( rank, rank, rank, rank );\n"
"}\n"
"//__attribute__((reqd_work_group_size(64,1,1)))\n"
"uint4 localPrefixSum64V( uint4 pData, uint lIdx, uint* totalSum, __local u32* sorterSharedMemory )\n"
"{\n"
"	u32 s4 = prefixScanVectorEx( &pData );\n"
"	u32 rank = localPrefixSum( s4, lIdx, totalSum, sorterSharedMemory, 64 );\n"
"	return pData + make_uint4( rank, rank, rank, rank );\n"
"}\n"
"u32 unpack4Key( u32 key, int keyIdx ){ return (key>>(keyIdx*8)) & 0xff;}\n"
"u32 bit8Scan(u32 v)\n"
"{\n"
"	return (v<<8) + (v<<16) + (v<<24);\n"
"}\n"
"//===\n"
"#define MY_HISTOGRAM(idx) localHistogramMat[(idx)*WG_SIZE+lIdx]\n"
"__kernel\n"
"__attribute__((reqd_work_group_size(WG_SIZE,1,1)))\n"
"void StreamCountKernel( __global u32* gSrc, __global u32* histogramOut, int4 cb )\n"
"{\n"
"	__local u32 localHistogramMat[NUM_BUCKET*WG_SIZE];\n"
"	u32 gIdx = GET_GLOBAL_IDX;\n"
"	u32 lIdx = GET_LOCAL_IDX;\n"
"	u32 wgIdx = GET_GROUP_IDX;\n"
"	u32 wgSize = GET_GROUP_SIZE;\n"
"	const int startBit = cb.m_startBit;\n"
"	const int n = cb.m_n;\n"
"	const int nWGs = cb.m_nWGs;\n"
"	const int nBlocksPerWG = cb.m_nBlocksPerWG;\n"
"	for(int i=0; i<NUM_BUCKET; i++)\n"
"	{\n"
"		MY_HISTOGRAM(i) = 0;\n"
"	}\n"
"	GROUP_LDS_BARRIER;\n"
"	const int blockSize = ELEMENTS_PER_WORK_ITEM*WG_SIZE;\n"
"	u32 localKey;\n"
"	int nBlocks = (n)/blockSize - nBlocksPerWG*wgIdx;\n"
"	int addr = blockSize*nBlocksPerWG*wgIdx + ELEMENTS_PER_WORK_ITEM*lIdx;\n"
"	for(int iblock=0; iblock<min(nBlocksPerWG, nBlocks); iblock++, addr+=blockSize)\n"
"	{\n"
"		//	MY_HISTOGRAM( localKeys.x ) ++ is much expensive than atomic add as it requires read and write while atomics can just add on AMD\n"
"		//	Using registers didn't perform well. It seems like use localKeys to address requires a lot of alu ops\n"
"		//	AMD: AtomInc performs better while NV prefers ++\n"
"		for(int i=0; i<ELEMENTS_PER_WORK_ITEM; i++)\n"
"		{\n"
"#if defined(CHECK_BOUNDARY)\n"
"			if( addr+i < n )\n"
"#endif\n"
"			{\n"
"				localKey = (gSrc[addr+i]>>startBit) & 0xf;\n"
"#if defined(NV_GPU)\n"
"				MY_HISTOGRAM( localKey )++;\n"
"#else\n"
"				AtomInc( MY_HISTOGRAM( localKey ) );\n"
"#endif\n"
"			}\n"
"		}\n"
"	}\n"
"	GROUP_LDS_BARRIER;\n"
"	\n"
"	if( lIdx < NUM_BUCKET )\n"
"	{\n"
"		u32 sum = 0;\n"
"		for(int i=0; i<GET_GROUP_SIZE; i++)\n"
"		{\n"
"			sum += localHistogramMat[lIdx*WG_SIZE+(i+lIdx)%GET_GROUP_SIZE];\n"
"		}\n"
"		histogramOut[lIdx*nWGs+wgIdx] = sum;\n"
"	}\n"
"}\n"
"__kernel\n"
"__attribute__((reqd_work_group_size(WG_SIZE,1,1)))\n"
"void StreamCountSortDataKernel( __global SortDataCL* gSrc, __global u32* histogramOut, int4  cb )\n"
"{\n"
"	__local u32 localHistogramMat[NUM_BUCKET*WG_SIZE];\n"
"	u32 gIdx = GET_GLOBAL_IDX;\n"
"	u32 lIdx = GET_LOCAL_IDX;\n"
"	u32 wgIdx = GET_GROUP_IDX;\n"
"	u32 wgSize = GET_GROUP_SIZE;\n"
"	const int startBit = cb.m_startBit;\n"
"	const int n = cb.m_n;\n"
"	const int nWGs = cb.m_nWGs;\n"
"	const int nBlocksPerWG = cb.m_nBlocksPerWG;\n"
"	for(int i=0; i<NUM_BUCKET; i++)\n"
"	{\n"
"		MY_HISTOGRAM(i) = 0;\n"
"	}\n"
"	GROUP_LDS_BARRIER;\n"
"	const int blockSize = ELEMENTS_PER_WORK_ITEM*WG_SIZE;\n"
"	u32 localKey;\n"
"	int nBlocks = (n)/blockSize - nBlocksPerWG*wgIdx;\n"
"	int addr = blockSize*nBlocksPerWG*wgIdx + ELEMENTS_PER_WORK_ITEM*lIdx;\n"
"	for(int iblock=0; iblock<min(nBlocksPerWG, nBlocks); iblock++, addr+=blockSize)\n"
"	{\n"
"		//	MY_HISTOGRAM( localKeys.x ) ++ is much expensive than atomic add as it requires read and write while atomics can just add on AMD\n"
"		//	Using registers didn't perform well. It seems like use localKeys to address requires a lot of alu ops\n"
"		//	AMD: AtomInc performs better while NV prefers ++\n"
"		for(int i=0; i<ELEMENTS_PER_WORK_ITEM; i++)\n"
"		{\n"
"#if defined(CHECK_BOUNDARY)\n"
"			if( addr+i < n )\n"
"#endif\n"
"			{\n"
"				localKey = (gSrc[addr+i].m_key>>startBit) & 0xf;\n"
"#if defined(NV_GPU)\n"
"				MY_HISTOGRAM( localKey )++;\n"
"#else\n"
"				AtomInc( MY_HISTOGRAM( localKey ) );\n"
"#endif\n"
"			}\n"
"		}\n"
"	}\n"
"	GROUP_LDS_BARRIER;\n"
"	\n"
"	if( lIdx < NUM_BUCKET )\n"
"	{\n"
"		u32 sum = 0;\n"
"		for(int i=0; i<GET_GROUP_SIZE; i++)\n"
"		{\n"
"			sum += localHistogramMat[lIdx*WG_SIZE+(i+lIdx)%GET_GROUP_SIZE];\n"
"		}\n"
"		histogramOut[lIdx*nWGs+wgIdx] = sum;\n"
"	}\n"
"}\n"
"#define nPerLane (nPerWI/4)\n"
"//	NUM_BUCKET*nWGs < 128*nPerWI\n"
"__kernel\n"
"__attribute__((reqd_work_group_size(128,1,1)))\n"
"void PrefixScanKernel( __global u32* wHistogram1, int4  cb )\n"
"{\n"
"	__local u32 ldsTopScanData[128*2];\n"
"	u32 lIdx = GET_LOCAL_IDX;\n"
"	u32 wgIdx = GET_GROUP_IDX;\n"
"	const int nWGs = cb.m_nWGs;\n"
"	u32 data[nPerWI];\n"
"	for(int i=0; i<nPerWI; i++)\n"
"	{\n"
"		data[i] = 0;\n"
"		if( (nPerWI*lIdx+i) < NUM_BUCKET*nWGs )\n"
"			data[i] = wHistogram1[nPerWI*lIdx+i];\n"
"	}\n"
"	uint4 myData = make_uint4(0,0,0,0);\n"
"	for(int i=0; i<nPerLane; i++)\n"
"	{\n"
"		myData.x += data[nPerLane*0+i];\n"
"		myData.y += data[nPerLane*1+i];\n"
"		myData.z += data[nPerLane*2+i];\n"
"		myData.w += data[nPerLane*3+i];\n"
"	}\n"
"	uint totalSum;\n"
"	uint4 scanned = localPrefixSum128V( myData, lIdx, &totalSum, ldsTopScanData );\n"
"//	for(int j=0; j<4; j++) //	somehow it introduces a lot of branches\n"
"	{	int j = 0;\n"
"		u32 sum = 0;\n"
"		for(int i=0; i<nPerLane; i++)\n"
"		{\n"
"			u32 tmp = data[nPerLane*j+i];\n"
"			data[nPerLane*j+i] = sum;\n"
"			sum += tmp;\n"
"		}\n"
"	}\n"
"	{	int j = 1;\n"
"		u32 sum = 0;\n"
"		for(int i=0; i<nPerLane; i++)\n"
"		{\n"
"			u32 tmp = data[nPerLane*j+i];\n"
"			data[nPerLane*j+i] = sum;\n"
"			sum += tmp;\n"
"		}\n"
"	}\n"
"	{	int j = 2;\n"
"		u32 sum = 0;\n"
"		for(int i=0; i<nPerLane; i++)\n"
"		{\n"
"			u32 tmp = data[nPerLane*j+i];\n"
"			data[nPerLane*j+i] = sum;\n"
"			sum += tmp;\n"
"		}\n"
"	}\n"
"	{	int j = 3;\n"
"		u32 sum = 0;\n"
"		for(int i=0; i<nPerLane; i++)\n"
"		{\n"
"			u32 tmp = data[nPerLane*j+i];\n"
"			data[nPerLane*j+i] = sum;\n"
"			sum += tmp;\n"
"		}\n"
"	}\n"
"	for(int i=0; i<nPerLane; i++)\n"
"	{\n"
"		data[nPerLane*0+i] += scanned.x;\n"
"		data[nPerLane*1+i] += scanned.y;\n"
"		data[nPerLane*2+i] += scanned.z;\n"
"		data[nPerLane*3+i] += scanned.w;\n"
"	}\n"
"	for(int i=0; i<nPerWI; i++)\n"
"	{\n"
"		int index = nPerWI*lIdx+i;\n"
"		if (index < NUM_BUCKET*nWGs)\n"
"			wHistogram1[nPerWI*lIdx+i] = data[i];\n"
"	}\n"
"}\n"
"//	4 scan, 4 exchange\n"
"void sort4Bits(u32 sortData[4], int startBit, int lIdx, __local u32* ldsSortData)\n"
"{\n"
"	for(int bitIdx=0; bitIdx<BITS_PER_PASS; bitIdx++)\n"
"	{\n"
"		u32 mask = (1<<bitIdx);\n"
"		uint4 cmpResult = make_uint4( (sortData[0]>>startBit) & mask, (sortData[1]>>startBit) & mask, (sortData[2]>>startBit) & mask, (sortData[3]>>startBit) & mask );\n"
"		uint4 prefixSum = SELECT_UINT4( make_uint4(1,1,1,1), make_uint4(0,0,0,0), cmpResult != make_uint4(0,0,0,0) );\n"
"		u32 total;\n"
"		prefixSum = localPrefixSum64V( prefixSum, lIdx, &total, ldsSortData );\n"
"		{\n"
"			uint4 localAddr = make_uint4(lIdx*4+0,lIdx*4+1,lIdx*4+2,lIdx*4+3);\n"
"			uint4 dstAddr = localAddr - prefixSum + make_uint4( total, total, total, total );\n"
"			dstAddr = SELECT_UINT4( prefixSum, dstAddr, cmpResult != make_uint4(0, 0, 0, 0) );\n"
"			GROUP_LDS_BARRIER;\n"
"			ldsSortData[dstAddr.x] = sortData[0];\n"
"			ldsSortData[dstAddr.y] = sortData[1];\n"
"			ldsSortData[dstAddr.z] = sortData[2];\n"
"			ldsSortData[dstAddr.w] = sortData[3];\n"
"			GROUP_LDS_BARRIER;\n"
"			sortData[0] = ldsSortData[localAddr.x];\n"
"			sortData[1] = ldsSortData[localAddr.y];\n"
"			sortData[2] = ldsSortData[localAddr.z];\n"
"			sortData[3] = ldsSortData[localAddr.w];\n"
"			GROUP_LDS_BARRIER;\n"
"		}\n"
"	}\n"
"}\n"
"//	2 scan, 2 exchange\n"
"void sort4Bits1(u32 sortData[4], int startBit, int lIdx, __local u32* ldsSortData)\n"
"{\n"
"	for(uint ibit=0; ibit<BITS_PER_PASS; ibit+=2)\n"
"	{\n"
"		uint4 b = make_uint4((sortData[0]>>(startBit+ibit)) & 0x3, \n"
"			(sortData[1]>>(startBit+ibit)) & 0x3, \n"
"			(sortData[2]>>(startBit+ibit)) & 0x3, \n"
"			(sortData[3]>>(startBit+ibit)) & 0x3);\n"
"		u32 key4;\n"
"		u32 sKeyPacked[4] = { 0, 0, 0, 0 };\n"
"		{\n"
"			sKeyPacked[0] |= 1<<(8*b.x);\n"
"			sKeyPacked[1] |= 1<<(8*b.y);\n"
"			sKeyPacked[2] |= 1<<(8*b.z);\n"
"			sKeyPacked[3] |= 1<<(8*b.w);\n"
"			key4 = sKeyPacked[0] + sKeyPacked[1] + sKeyPacked[2] + sKeyPacked[3];\n"
"		}\n"
"		u32 rankPacked;\n"
"		u32 sumPacked;\n"
"		{\n"
"			rankPacked = localPrefixSum( key4, lIdx, &sumPacked, ldsSortData, WG_SIZE );\n"
"		}\n"
"		GROUP_LDS_BARRIER;\n"
"		u32 newOffset[4] = { 0,0,0,0 };\n"
"		{\n"
"			u32 sumScanned = bit8Scan( sumPacked );\n"
"			u32 scannedKeys[4];\n"
"			scannedKeys[0] = 1<<(8*b.x);\n"
"			scannedKeys[1] = 1<<(8*b.y);\n"
"			scannedKeys[2] = 1<<(8*b.z);\n"
"			scannedKeys[3] = 1<<(8*b.w);\n"
"			{	//	4 scans at once\n"
"				u32 sum4 = 0;\n"
"				for(int ie=0; ie<4; ie++)\n"
"				{\n"
"					u32 tmp = scannedKeys[ie];\n"
"					scannedKeys[ie] = sum4;\n"
"					sum4 += tmp;\n"
"				}\n"
"			}\n"
"			{\n"
"				u32 sumPlusRank = sumScanned + rankPacked;\n"
"				{	u32 ie = b.x;\n"
"					scannedKeys[0] += sumPlusRank;\n"
"					newOffset[0] = unpack4Key( scannedKeys[0], ie );\n"
"				}\n"
"				{	u32 ie = b.y;\n"
"					scannedKeys[1] += sumPlusRank;\n"
"					newOffset[1] = unpack4Key( scannedKeys[1], ie );\n"
"				}\n"
"				{	u32 ie = b.z;\n"
"					scannedKeys[2] += sumPlusRank;\n"
"					newOffset[2] = unpack4Key( scannedKeys[2], ie );\n"
"				}\n"
"				{	u32 ie = b.w;\n"
"					scannedKeys[3] += sumPlusRank;\n"
"					newOffset[3] = unpack4Key( scannedKeys[3], ie );\n"
"				}\n"
"			}\n"
"		}\n"
"		GROUP_LDS_BARRIER;\n"
"		{\n"
"			ldsSortData[newOffset[0]] = sortData[0];\n"
"			ldsSortData[newOffset[1]] = sortData[1];\n"
"			ldsSortData[newOffset[2]] = sortData[2];\n"
"			ldsSortData[newOffset[3]] = sortData[3];\n"
"			GROUP_LDS_BARRIER;\n"
"			u32 dstAddr = 4*lIdx;\n"
"			sortData[0] = ldsSortData[dstAddr+0];\n"
"			sortData[1] = ldsSortData[dstAddr+1];\n"
"			sortData[2] = ldsSortData[dstAddr+2];\n"
"			sortData[3] = ldsSortData[dstAddr+3];\n"
"			GROUP_LDS_BARRIER;\n"
"		}\n"
"	}\n"
"}\n"
"#define SET_HISTOGRAM(setIdx, key) ldsSortData[(setIdx)*NUM_BUCKET+key]\n"
"__kernel\n"
"__attribute__((reqd_work_group_size(WG_SIZE,1,1)))\n"
"void SortAndScatterKernel( __global const u32* restrict gSrc, __global const u32* rHistogram, __global u32* restrict gDst, int4  cb )\n"
"{\n"
"	__local u32 ldsSortData[WG_SIZE*ELEMENTS_PER_WORK_ITEM+16];\n"
"	__local u32 localHistogramToCarry[NUM_BUCKET];\n"
"	__local u32 localHistogram[NUM_BUCKET*2];\n"
"	u32 gIdx = GET_GLOBAL_IDX;\n"
"	u32 lIdx = GET_LOCAL_IDX;\n"
"	u32 wgIdx = GET_GROUP_IDX;\n"
"	u32 wgSize = GET_GROUP_SIZE;\n"
"	const int n = cb.m_n;\n"
"	const int nWGs = cb.m_nWGs;\n"
"	const int startBit = cb.m_startBit;\n"
"	const int nBlocksPerWG = cb.m_nBlocksPerWG;\n"
"	if( lIdx < (NUM_BUCKET) )\n"
"	{\n"
"		localHistogramToCarry[lIdx] = rHistogram[lIdx*nWGs + wgIdx];\n"
"	}\n"
"	GROUP_LDS_BARRIER;\n"
"	const int blockSize = ELEMENTS_PER_WORK_ITEM*WG_SIZE;\n"
"	int nBlocks = n/blockSize - nBlocksPerWG*wgIdx;\n"
"	int addr = blockSize*nBlocksPerWG*wgIdx + ELEMENTS_PER_WORK_ITEM*lIdx;\n"
"	for(int iblock=0; iblock<min(nBlocksPerWG, nBlocks); iblock++, addr+=blockSize)\n"
"	{\n"
"		u32 myHistogram = 0;\n"
"		u32 sortData[ELEMENTS_PER_WORK_ITEM];\n"
"		for(int i=0; i<ELEMENTS_PER_WORK_ITEM; i++)\n"
"#if defined(CHECK_BOUNDARY)\n"
"			sortData[i] = ( addr+i < n )? gSrc[ addr+i ] : 0xffffffff;\n"
"#else\n"
"			sortData[i] = gSrc[ addr+i ];\n"
"#endif\n"
"		sort4Bits(sortData, startBit, lIdx, ldsSortData);\n"
"		u32 keys[ELEMENTS_PER_WORK_ITEM];\n"
"		for(int i=0; i<ELEMENTS_PER_WORK_ITEM; i++)\n"
"			keys[i] = (sortData[i]>>startBit) & 0xf;\n"
"		{	//	create histogram\n"
"			u32 setIdx = lIdx/16;\n"
"			if( lIdx < NUM_BUCKET )\n"
"			{\n"
"				localHistogram[lIdx] = 0;\n"
"			}\n"
"			ldsSortData[lIdx] = 0;\n"
"			GROUP_LDS_BARRIER;\n"
"			for(int i=0; i<ELEMENTS_PER_WORK_ITEM; i++)\n"
"#if defined(CHECK_BOUNDARY)\n"
"				if( addr+i < n )\n"
"#endif\n"
"#if defined(NV_GPU)\n"
"				SET_HISTOGRAM( setIdx, keys[i] )++;\n"
"#else\n"
"				AtomInc( SET_HISTOGRAM( setIdx, keys[i] ) );\n"
"#endif\n"
"			\n"
"			GROUP_LDS_BARRIER;\n"
"			\n"
"			uint hIdx = NUM_BUCKET+lIdx;\n"
"			if( lIdx < NUM_BUCKET )\n"
"			{\n"
"				u32 sum = 0;\n"
"				for(int i=0; i<WG_SIZE/16; i++)\n"
"				{\n"
"					sum += SET_HISTOGRAM( i, lIdx );\n"
"				}\n"
"				myHistogram = sum;\n"
"				localHistogram[hIdx] = sum;\n"
"			}\n"
"			GROUP_LDS_BARRIER;\n"
"#if defined(USE_2LEVEL_REDUCE)\n"
"			if( lIdx < NUM_BUCKET )\n"
"			{\n"
"				localHistogram[hIdx] = localHistogram[hIdx-1];\n"
"				GROUP_MEM_FENCE;\n"
"				u32 u0, u1, u2;\n"
"				u0 = localHistogram[hIdx-3];\n"
"				u1 = localHistogram[hIdx-2];\n"
"				u2 = localHistogram[hIdx-1];\n"
"				AtomAdd( localHistogram[hIdx], u0 + u1 + u2 );\n"
"				GROUP_MEM_FENCE;\n"
"				u0 = localHistogram[hIdx-12];\n"
"				u1 = localHistogram[hIdx-8];\n"
"				u2 = localHistogram[hIdx-4];\n"
"				AtomAdd( localHistogram[hIdx], u0 + u1 + u2 );\n"
"				GROUP_MEM_FENCE;\n"
"			}\n"
"#else\n"
"			if( lIdx < NUM_BUCKET )\n"
"			{\n"
"				localHistogram[hIdx] = localHistogram[hIdx-1];\n"
"				GROUP_MEM_FENCE;\n"
"				localHistogram[hIdx] += localHistogram[hIdx-1];\n"
"				GROUP_MEM_FENCE;\n"
"				localHistogram[hIdx] += localHistogram[hIdx-2];\n"
"				GROUP_MEM_FENCE;\n"
"				localHistogram[hIdx] += localHistogram[hIdx-4];\n"
"				GROUP_MEM_FENCE;\n"
"				localHistogram[hIdx] += localHistogram[hIdx-8];\n"
"				GROUP_MEM_FENCE;\n"
"			}\n"
"#endif\n"
"			GROUP_LDS_BARRIER;\n"
"		}\n"
"		{\n"
"			for(int ie=0; ie<ELEMENTS_PER_WORK_ITEM; ie++)\n"
"			{\n"
"				int dataIdx = ELEMENTS_PER_WORK_ITEM*lIdx+ie;\n"
"				int binIdx = keys[ie];\n"
"				int groupOffset = localHistogramToCarry[binIdx];\n"
"				int myIdx = dataIdx - localHistogram[NUM_BUCKET+binIdx];\n"
"#if defined(CHECK_BOUNDARY)\n"
"				if( addr+ie < n )\n"
"#endif\n"
"				gDst[ groupOffset + myIdx ] = sortData[ie];\n"
"			}\n"
"		}\n"
"		GROUP_LDS_BARRIER;\n"
"		if( lIdx < NUM_BUCKET )\n"
"		{\n"
"			localHistogramToCarry[lIdx] += myHistogram;\n"
"		}\n"
"		GROUP_LDS_BARRIER;\n"
"	}\n"
"}\n"
"//	2 scan, 2 exchange\n"
"void sort4Bits1KeyValue(u32 sortData[4], int sortVal[4], int startBit, int lIdx, __local u32* ldsSortData, __local int *ldsSortVal)\n"
"{\n"
"	for(uint ibit=0; ibit<BITS_PER_PASS; ibit+=2)\n"
"	{\n"
"		uint4 b = make_uint4((sortData[0]>>(startBit+ibit)) & 0x3, \n"
"			(sortData[1]>>(startBit+ibit)) & 0x3, \n"
"			(sortData[2]>>(startBit+ibit)) & 0x3, \n"
"			(sortData[3]>>(startBit+ibit)) & 0x3);\n"
"		u32 key4;\n"
"		u32 sKeyPacked[4] = { 0, 0, 0, 0 };\n"
"		{\n"
"			sKeyPacked[0] |= 1<<(8*b.x);\n"
"			sKeyPacked[1] |= 1<<(8*b.y);\n"
"			sKeyPacked[2] |= 1<<(8*b.z);\n"
"			sKeyPacked[3] |= 1<<(8*b.w);\n"
"			key4 = sKeyPacked[0] + sKeyPacked[1] + sKeyPacked[2] + sKeyPacked[3];\n"
"		}\n"
"		u32 rankPacked;\n"
"		u32 sumPacked;\n"
"		{\n"
"			rankPacked = localPrefixSum( key4, lIdx, &sumPacked, ldsSortData, WG_SIZE );\n"
"		}\n"
"		GROUP_LDS_BARRIER;\n"
"		u32 newOffset[4] = { 0,0,0,0 };\n"
"		{\n"
"			u32 sumScanned = bit8Scan( sumPacked );\n"
"			u32 scannedKeys[4];\n"
"			scannedKeys[0] = 1<<(8*b.x);\n"
"			scannedKeys[1] = 1<<(8*b.y);\n"
"			scannedKeys[2] = 1<<(8*b.z);\n"
"			scannedKeys[3] = 1<<(8*b.w);\n"
"			{	//	4 scans at once\n"
"				u32 sum4 = 0;\n"
"				for(int ie=0; ie<4; ie++)\n"
"				{\n"
"					u32 tmp = scannedKeys[ie];\n"
"					scannedKeys[ie] = sum4;\n"
"					sum4 += tmp;\n"
"				}\n"
"			}\n"
"			{\n"
"				u32 sumPlusRank = sumScanned + rankPacked;\n"
"				{	u32 ie = b.x;\n"
"					scannedKeys[0] += sumPlusRank;\n"
"					newOffset[0] = unpack4Key( scannedKeys[0], ie );\n"
"				}\n"
"				{	u32 ie = b.y;\n"
"					scannedKeys[1] += sumPlusRank;\n"
"					newOffset[1] = unpack4Key( scannedKeys[1], ie );\n"
"				}\n"
"				{	u32 ie = b.z;\n"
"					scannedKeys[2] += sumPlusRank;\n"
"					newOffset[2] = unpack4Key( scannedKeys[2], ie );\n"
"				}\n"
"				{	u32 ie = b.w;\n"
"					scannedKeys[3] += sumPlusRank;\n"
"					newOffset[3] = unpack4Key( scannedKeys[3], ie );\n"
"				}\n"
"			}\n"
"		}\n"
"		GROUP_LDS_BARRIER;\n"
"		{\n"
"			ldsSortData[newOffset[0]] = sortData[0];\n"
"			ldsSortData[newOffset[1]] = sortData[1];\n"
"			ldsSortData[newOffset[2]] = sortData[2];\n"
"			ldsSortData[newOffset[3]] = sortData[3];\n"
"			ldsSortVal[newOffset[0]] = sortVal[0];\n"
"			ldsSortVal[newOffset[1]] = sortVal[1];\n"
"			ldsSortVal[newOffset[2]] = sortVal[2];\n"
"			ldsSortVal[newOffset[3]] = sortVal[3];\n"
"			GROUP_LDS_BARRIER;\n"
"			u32 dstAddr = 4*lIdx;\n"
"			sortData[0] = ldsSortData[dstAddr+0];\n"
"			sortData[1] = ldsSortData[dstAddr+1];\n"
"			sortData[2] = ldsSortData[dstAddr+2];\n"
"			sortData[3] = ldsSortData[dstAddr+3];\n"
"			sortVal[0] = ldsSortVal[dstAddr+0];\n"
"			sortVal[1] = ldsSortVal[dstAddr+1];\n"
"			sortVal[2] = ldsSortVal[dstAddr+2];\n"
"			sortVal[3] = ldsSortVal[dstAddr+3];\n"
"			GROUP_LDS_BARRIER;\n"
"		}\n"
"	}\n"
"}\n"
"__kernel\n"
"__attribute__((reqd_work_group_size(WG_SIZE,1,1)))\n"
"void SortAndScatterSortDataKernel( __global const SortDataCL* restrict gSrc, __global const u32* rHistogram, __global SortDataCL* restrict gDst, int4 cb)\n"
"{\n"
"	__local int ldsSortData[WG_SIZE*ELEMENTS_PER_WORK_ITEM+16];\n"
"	__local int ldsSortVal[WG_SIZE*ELEMENTS_PER_WORK_ITEM+16];\n"
"	__local u32 localHistogramToCarry[NUM_BUCKET];\n"
"	__local u32 localHistogram[NUM_BUCKET*2];\n"
"	u32 gIdx = GET_GLOBAL_IDX;\n"
"	u32 lIdx = GET_LOCAL_IDX;\n"
"	u32 wgIdx = GET_GROUP_IDX;\n"
"	u32 wgSize = GET_GROUP_SIZE;\n"
"	const int n = cb.m_n;\n"
"	const int nWGs = cb.m_nWGs;\n"
"	const int startBit = cb.m_startBit;\n"
"	const int nBlocksPerWG = cb.m_nBlocksPerWG;\n"
"	if( lIdx < (NUM_BUCKET) )\n"
"	{\n"
"		localHistogramToCarry[lIdx] = rHistogram[lIdx*nWGs + wgIdx];\n"
"	}\n"
"	GROUP_LDS_BARRIER;\n"
"    \n"
"	const int blockSize = ELEMENTS_PER_WORK_ITEM*WG_SIZE;\n"
"	int nBlocks = n/blockSize - nBlocksPerWG*wgIdx;\n"
"	int addr = blockSize*nBlocksPerWG*wgIdx + ELEMENTS_PER_WORK_ITEM*lIdx;\n"
"	for(int iblock=0; iblock<min(nBlocksPerWG, nBlocks); iblock++, addr+=blockSize)\n"
"	{\n"
"		u32 myHistogram = 0;\n"
"		int sortData[ELEMENTS_PER_WORK_ITEM];\n"
"		int sortVal[ELEMENTS_PER_WORK_ITEM];\n"
"		for(int i=0; i<ELEMENTS_PER_WORK_ITEM; i++)\n"
"#if defined(CHECK_BOUNDARY)\n"
"		{\n"
"			sortData[i] = ( addr+i < n )? gSrc[ addr+i ].m_key : 0xffffffff;\n"
"			sortVal[i] = ( addr+i < n )? gSrc[ addr+i ].m_value : 0xffffffff;\n"
"		}\n"
"#else\n"
"		{\n"
"			sortData[i] = gSrc[ addr+i ].m_key;\n"
"			sortVal[i] = gSrc[ addr+i ].m_value;\n"
"		}\n"
"#endif\n"
"		sort4Bits1KeyValue(sortData, sortVal, startBit, lIdx, ldsSortData, ldsSortVal);\n"
"		u32 keys[ELEMENTS_PER_WORK_ITEM];\n"
"		for(int i=0; i<ELEMENTS_PER_WORK_ITEM; i++)\n"
"			keys[i] = (sortData[i]>>startBit) & 0xf;\n"
"		{	//	create histogram\n"
"			u32 setIdx = lIdx/16;\n"
"			if( lIdx < NUM_BUCKET )\n"
"			{\n"
"				localHistogram[lIdx] = 0;\n"
"			}\n"
"			ldsSortData[lIdx] = 0;\n"
"			GROUP_LDS_BARRIER;\n"
"			for(int i=0; i<ELEMENTS_PER_WORK_ITEM; i++)\n"
"#if defined(CHECK_BOUNDARY)\n"
"				if( addr+i < n )\n"
"#endif\n"
"#if defined(NV_GPU)\n"
"				SET_HISTOGRAM( setIdx, keys[i] )++;\n"
"#else\n"
"				AtomInc( SET_HISTOGRAM( setIdx, keys[i] ) );\n"
"#endif\n"
"			\n"
"			GROUP_LDS_BARRIER;\n"
"			\n"
"			uint hIdx = NUM_BUCKET+lIdx;\n"
"			if( lIdx < NUM_BUCKET )\n"
"			{\n"
"				u32 sum = 0;\n"
"				for(int i=0; i<WG_SIZE/16; i++)\n"
"				{\n"
"					sum += SET_HISTOGRAM( i, lIdx );\n"
"				}\n"
"				myHistogram = sum;\n"
"				localHistogram[hIdx] = sum;\n"
"			}\n"
"			GROUP_LDS_BARRIER;\n"
"#if defined(USE_2LEVEL_REDUCE)\n"
"			if( lIdx < NUM_BUCKET )\n"
"			{\n"
"				localHistogram[hIdx] = localHistogram[hIdx-1];\n"
"				GROUP_MEM_FENCE;\n"
"				u32 u0, u1, u2;\n"
"				u0 = localHistogram[hIdx-3];\n"
"				u1 = localHistogram[hIdx-2];\n"
"				u2 = localHistogram[hIdx-1];\n"
"				AtomAdd( localHistogram[hIdx], u0 + u1 + u2 );\n"
"				GROUP_MEM_FENCE;\n"
"				u0 = localHistogram[hIdx-12];\n"
"				u1 = localHistogram[hIdx-8];\n"
"				u2 = localHistogram[hIdx-4];\n"
"				AtomAdd( localHistogram[hIdx], u0 + u1 + u2 );\n"
"				GROUP_MEM_FENCE;\n"
"			}\n"
"#else\n"
"			if( lIdx < NUM_BUCKET )\n"
"			{\n"
"				localHistogram[hIdx] = localHistogram[hIdx-1];\n"
"				GROUP_MEM_FENCE;\n"
"				localHistogram[hIdx] += localHistogram[hIdx-1];\n"
"				GROUP_MEM_FENCE;\n"
"				localHistogram[hIdx] += localHistogram[hIdx-2];\n"
"				GROUP_MEM_FENCE;\n"
"				localHistogram[hIdx] += localHistogram[hIdx-4];\n"
"				GROUP_MEM_FENCE;\n"
"				localHistogram[hIdx] += localHistogram[hIdx-8];\n"
"				GROUP_MEM_FENCE;\n"
"			}\n"
"#endif\n"
"			GROUP_LDS_BARRIER;\n"
"		}\n"
"    	{\n"
"			for(int ie=0; ie<ELEMENTS_PER_WORK_ITEM; ie++)\n"
"			{\n"
"				int dataIdx = ELEMENTS_PER_WORK_ITEM*lIdx+ie;\n"
"				int binIdx = keys[ie];\n"
"				int groupOffset = localHistogramToCarry[binIdx];\n"
"				int myIdx = dataIdx - localHistogram[NUM_BUCKET+binIdx];\n"
"#if defined(CHECK_BOUNDARY)\n"
"				if( addr+ie < n )\n"
"				{\n"
"                    if ((groupOffset + myIdx)<n)\n"
"                    {\n"
"                        if (sortData[ie]==sortVal[ie])\n"
"                        {\n"
"                            \n"
"                            SortDataCL tmp;\n"
"                            tmp.m_key = sortData[ie];\n"
"                            tmp.m_value = sortVal[ie];\n"
"                            if (tmp.m_key == tmp.m_value)\n"
"                                gDst[groupOffset + myIdx ] = tmp;\n"
"                        }\n"
"                        \n"
"                    }\n"
"				}\n"
"#else\n"
"                if ((groupOffset + myIdx)<n)\n"
"                {\n"
"                    gDst[ groupOffset + myIdx ].m_key = sortData[ie];\n"
"                    gDst[ groupOffset + myIdx ].m_value = sortVal[ie];\n"
"                }\n"
"#endif\n"
"			}\n"
"		}\n"
"		GROUP_LDS_BARRIER;\n"
"		if( lIdx < NUM_BUCKET )\n"
"		{\n"
"			localHistogramToCarry[lIdx] += myHistogram;\n"
"		}\n"
"		GROUP_LDS_BARRIER;\n"
"	}\n"
"}\n"
"__kernel\n"
"__attribute__((reqd_work_group_size(WG_SIZE,1,1)))\n"
"void SortAndScatterSortDataKernelSerial( __global const SortDataCL* restrict gSrc, __global const u32* rHistogram, __global SortDataCL* restrict gDst, int4 cb)\n"
"{\n"
"    \n"
"	u32 gIdx = GET_GLOBAL_IDX;\n"
"	u32 realLocalIdx = GET_LOCAL_IDX;\n"
"	u32 wgIdx = GET_GROUP_IDX;\n"
"	u32 wgSize = GET_GROUP_SIZE;\n"
"	const int startBit = cb.m_startBit;\n"
"	const int n = cb.m_n;\n"
"	const int nWGs = cb.m_nWGs;\n"
"	const int nBlocksPerWG = cb.m_nBlocksPerWG;\n"
"    int counter[NUM_BUCKET];\n"
"    \n"
"    if (realLocalIdx>0)\n"
"        return;\n"
"    \n"
"    for (int c=0;c<NUM_BUCKET;c++)\n"
"        counter[c]=0;\n"
"    const int blockSize = ELEMENTS_PER_WORK_ITEM*WG_SIZE;\n"
"	\n"
"	int nBlocks = (n)/blockSize - nBlocksPerWG*wgIdx;\n"
"   for(int iblock=0; iblock<min(nBlocksPerWG, nBlocks); iblock++)\n"
"  {\n"
"     for (int lIdx=0;lIdx<WG_SIZE;lIdx++)\n"
" 	{\n"
"        int addr2 = iblock*blockSize + blockSize*nBlocksPerWG*wgIdx + ELEMENTS_PER_WORK_ITEM*lIdx;\n"
"        \n"
"		for(int j=0; j<ELEMENTS_PER_WORK_ITEM; j++)\n"
"		{\n"
"            int i = addr2+j;\n"
"			if( i < n )\n"
"			{\n"
"                int tableIdx;\n"
"				tableIdx = (gSrc[i].m_key>>startBit) & 0xf;//0xf = NUM_TABLES-1\n"
"                gDst[rHistogram[tableIdx*nWGs+wgIdx] + counter[tableIdx]] = gSrc[i];\n"
"                counter[tableIdx] ++;\n"
"			}\n"
"		}\n"
"	}\n"
"  }\n"
"    \n"
"}\n"
"__kernel\n"
"__attribute__((reqd_work_group_size(WG_SIZE,1,1)))\n"
"void SortAndScatterKernelSerial( __global const u32* restrict gSrc, __global const u32* rHistogram, __global u32* restrict gDst, int4  cb )\n"
"{\n"
"    \n"
"	u32 gIdx = GET_GLOBAL_IDX;\n"
"	u32 realLocalIdx = GET_LOCAL_IDX;\n"
"	u32 wgIdx = GET_GROUP_IDX;\n"
"	u32 wgSize = GET_GROUP_SIZE;\n"
"	const int startBit = cb.m_startBit;\n"
"	const int n = cb.m_n;\n"
"	const int nWGs = cb.m_nWGs;\n"
"	const int nBlocksPerWG = cb.m_nBlocksPerWG;\n"
"    int counter[NUM_BUCKET];\n"
"    \n"
"    if (realLocalIdx>0)\n"
"        return;\n"
"    \n"
"    for (int c=0;c<NUM_BUCKET;c++)\n"
"        counter[c]=0;\n"
"    const int blockSize = ELEMENTS_PER_WORK_ITEM*WG_SIZE;\n"
"	\n"
"	int nBlocks = (n)/blockSize - nBlocksPerWG*wgIdx;\n"
"   for(int iblock=0; iblock<min(nBlocksPerWG, nBlocks); iblock++)\n"
"  {\n"
"     for (int lIdx=0;lIdx<WG_SIZE;lIdx++)\n"
" 	{\n"
"        int addr2 = iblock*blockSize + blockSize*nBlocksPerWG*wgIdx + ELEMENTS_PER_WORK_ITEM*lIdx;\n"
"        \n"
"		for(int j=0; j<ELEMENTS_PER_WORK_ITEM; j++)\n"
"		{\n"
"            int i = addr2+j;\n"
"			if( i < n )\n"
"			{\n"
"                int tableIdx;\n"
"				tableIdx = (gSrc[i]>>startBit) & 0xf;//0xf = NUM_TABLES-1\n"
"                gDst[rHistogram[tableIdx*nWGs+wgIdx] + counter[tableIdx]] = gSrc[i];\n"
"                counter[tableIdx] ++;\n"
"			}\n"
"		}\n"
"	}\n"
"  }\n"
"    \n"
"}\n"
;
