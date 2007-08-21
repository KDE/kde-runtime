#ifndef __BITMAPS_H
#define __BITMAPS_H

/* Image bits processed by KPixmap2Bitmaps */
// Arrow bitmaps
static const QCOORD u_arrow[]={-1,-3, 0,-3, -2,-2, 1,-2, -3,-1, 2,-1, -4,0, 3,0, -4,1, 3,1};
static const QCOORD d_arrow[]={-4,-2, 3,-2, -4,-1, 3,-1, -3,0, 2,0, -2,1, 1,1, -1,2, 0,2};
static const QCOORD l_arrow[]={-3,-1, -3,0, -2,-2, -2,1, -1,-3, -1,2, 0,-4, 0,3, 1,-4, 1,3};
static const QCOORD r_arrow[]={-2,-4, -2,3, -1,-4, -1,3, 0,-3, 0,2, 1,-2, 1,1, 2,-1, 2,0};

static const QCOORD keramik_combo_arrow[] =
	{-4,-5, 4, -5,
	 -2 ,-2, 2, -2,
	  -2 ,-1, 2, -1,
	   -2 ,0, 2, 0,
	   -4, 1, 4, 1,
	   -3, 2, 3, 2,
	   -2 , 3, 2, 3,
	   -1 , 4, 1, 4,
	   0 , 5, 0, 5
	    };
	    

static const QCOORD keramik_up_arrow[] = 
	{
		0, -4, 0, -4,
		-1, -3, 1, -3,
		-2, -2, 2, -2,
		-3, -1, 3, -1,
		-4, 0, 4, 0,
		-2, 1, 2, 1,
		-2, 2, 2, 2,
		-2, 3, 2, 3,
		-2, 4, 2, 4
	};
	
static const QCOORD keramik_down_arrow[] = 
	{
		0, 4, 0, 4,
		-1, 3, 1, 3,
		-2, 2, 2, 2,
		-3, 1, 3, 1,
		-4, 0, 4, 0,
		-2, -1, 2, -1,
		-2, -2, 2, -2,
		-2, -3, 2, -3,
		-2, -4, 2, -4
	};


	static const QCOORD keramik_right_arrow[] = 
	{
		4, 0, 4, 0,
		3, -1, 3, 1,
		2, -2, 2, 2,
		1, -3, 1, 3,
		0, -4, 0, 4,
		-1, -2, -1, 2,
		-2, -2, -2, 2,
		-3, -2, -3, 2,
		-4, -2, -4, 2
	};
	
	static const QCOORD keramik_left_arrow[] = 
	{
		-4, 0, -4, 0,
		-3, -1, -3, 1,
		-2, -2, -2, 2,
		-1, -3, -1, 3,
		0, -4, 0, 4,
		1, -2, 1, 2,
		2, -2, 2, 2,
		3, -2, 3, 2,
		4, -2, 4, 2
	};

	
		
#define QCOORDARRLEN(x) sizeof(x)/(sizeof(QCOORD)*2)



#endif
