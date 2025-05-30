#ifndef __REGION_H__
#define __REGION_H__
#include <GL/glu.h>
#include "VMath.h"

/**
 * Minimal renderable part of Earth
 *
 * Region is rectangular piece of earth surface, with sides parallel to parallels and meridians.
 * Thus, it has only world coordinates w[0] is lower left and w[1] is top right (height is likely
 * to be added as well). However, this rectangle may be rendered to screen in almost arbitary way,
 * (twisted, skewed, rotated etc.) - so virtual coordinates for each vertex are also stored.
 * Vertices are counted [0..3] beginning from bottom-left (that one that is w[0]), CCW.
 * Also projected coordinates may be used.
 */
class Region {
public:
	/**
	 * Constructor
	 */
	Region() {
	}

	/**
	 * Constructor with all put projected coords given
	 */
	Region(Vector3d v0, Vector3d v1, Vector3d v2, Vector3d v3, Vector2d w0, Vector2d w1) {
		v[0] = v0; v[1] = v1; v[2] = v2; v[3] = v3;
		w[0] = w0; w[1] = w1;
	}

	/**
	 * Constructor with all coordinates given
	 */
	Region(Vector3d v0, Vector3d v1, Vector3d v2, Vector3d v3, Vector2d w0, Vector2d w1, Vector3d p0, Vector3d p1, Vector3d p2, Vector3d p3) {
		v[0] = v0; v[1] = v1; v[2] = v2; v[3] = v3;
		w[0] = w0; w[1] = w1;
		p[0] = p0; p[1] = p1; p[2] = p2; p[3] = p3;
	}

	/**
	 * Calculate projected coordinates
	 */
	void calc_proj(const GLdouble *model, const GLdouble *proj, const GLint *view) {
		gluProject(v[0].x, v[0].y, v[0].z, model, proj, view, &p[0].x, &p[0].y, &p[0].z);
		gluProject(v[1].x, v[1].y, v[1].z, model, proj, view, &p[1].x, &p[1].y, &p[1].z);
		gluProject(v[2].x, v[2].y, v[2].z, model, proj, view, &p[2].x, &p[2].y, &p[2].z);
		gluProject(v[3].x, v[3].y, v[3].z, model, proj, view, &p[3].x, &p[3].y, &p[3].z);

		p[0].z = p[1].z = p[2].z = p[3].z = 0.0;
	}

	/**
	 * Reset Z-values of projected coords
	 */
	void reset_proj_z() {
		p[0].z = p[1].z = p[2].z = p[3].z = 0.0;
	}

	/**
	 * Length of edge (i,j)
	 */
	inline double proj_length(int i, int j) {
		return sqrt( (p[i].x - p[j].x) * (p[i].x - p[j].x) + (p[i].y - p[j].y) * (p[i].y - p[j].y) );
	}

public:
	Vector3d v[4];	///< Virtual coordinates (used in rendering)
	Vector2d w[2];	///< World coordinates (corners of parallel/meridian - oriented rectangle)
	Vector3d p[4];	///< Virtual coordinates after projection to screen (in pixels)
};

#endif
