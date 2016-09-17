#ifndef PYFR_CATALYSTDATA_H
#define PYFR_CATALYSTDATA_H

#include <inttypes.h>
#include <cuda_runtime.h>

/*
 * The vertex and solution arrays are three dimensional having a
 * format of [nb][X][na] where X = 3 for the vertex array (x, y, z)
 * and X = 5 for the solution array (rho, rhou, rhov, rhow, E).  The
 * total number of nodes is na*nb.
 *
 * The solution array is a device pointer.  To make things more
 * interesting it is also padded.  The distance between [i][j][k] and
 * [i + 1][j][k] is ldim elements.  The distance between [i][j][k] and
 * [i][j + 1][k] is lsdim elements.  Fun for the entire family!
 *
 * The total number of linear VTK elements is given by nel.  The type
 * of the i'th VTK element is given by type[i].  The offset of its node
 * numbers in the connectivity array is given by off[i] with the number
 * of nodes being determined by the type.  The node number
 * associated with the first vertex is therefore con[off[i]].  Care
 * must be taken when looking up this node in vert/soln due to their
 * three dimensional structure.
 */
struct MeshDataForCellType
{
  int nVerticesPerCell;
  int nCells;
  void* vertices;
  int nSubdividedCells; /* number of elements in con, off, type */
  int32_t* con;
  int32_t* off; /* offset of connectivity information in con[] */
  uint8_t* type;
};

struct SolutionDataForCellType
{
  int ldim;
  int lsdim;
  void* solution; /* "soln" in PyFR parlance. */
};

struct CatalystData
{
  int nCellTypes;
  MeshDataForCellType* meshData;
  SolutionDataForCellType* solutionData;
  float* isovalues; /* isovalues to compute */
  unsigned niso; /* number of isovalues in above array */
  bool metadata; /* should we print out per-TS metadata? */
  float* eye; /* 3-tuple, camera position */
  float* ref; /* 3-tuple, camera reference point */
  float* vup; /* 3-tuple, view up vector */
};
#endif
