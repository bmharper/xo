
//#ifndef MIX_NO_AXIS_NAMES
//enum Axis {X=0, Y=1, Z=2, W=3};
//#define MIX_NO_AXIS_NAMES
//#endif
#define X (0)
#define Y (1)
#define Z (2)
#define W (3)

// in matrix, which column mean Right, Up, and Normal
#define UL_R (X)
#define UL_U (Y)
#define UL_N (Z)

// in local coordinates, which is on the plane (RX,RY) and which is up (RZ)
// makes the local 2D calculations easier
#define RX (X)
#define RY (Z)
#define RZ (Y)

#define XX row[0].x
#define XY row[0].y
#define XZ row[0].z
#define XW row[0].w

#define YX row[1].x
#define YY row[1].y
#define YZ row[1].z
#define YW row[1].w

#define ZX row[2].x
#define ZY row[2].y
#define ZZ row[2].z
#define ZW row[2].w

#define WX row[3].x
#define WY row[3].y
#define WZ row[3].z
#define WW row[3].w

