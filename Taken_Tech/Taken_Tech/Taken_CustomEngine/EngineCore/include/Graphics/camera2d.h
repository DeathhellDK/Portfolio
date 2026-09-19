#pragma once
/**
 * @file    camera2d.h
 * @author  Jethro
 * @email     w.kyele, sung.h
 * @co-author Woh Kye Le
 * @date    2025-09-29
 *
 * @brief   Declares the Camera2D class for managing 2D view and projection.
 *
 * Camera2D handles orthographic projection, viewport transformations,
 * and view transforms (position, rotation, zoom). It provides utility
 * methods for converting between screen and world coordinates, and
 * for building MVP matrices for rendering.
 *
 * @version 1.0
 * @copyright
 * Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */
#include "Math/matrix3x3.h"
#include "Math/vect2.h"

 /**
  * @class Camera2D
  * @brief A 2D camera supporting translation, rotation, zoom, and projection.
  *
  * Provides functions to configure orthographic projection, viewport scaling,
  * and to retrieve combined matrices for rendering. Also enables converting
  * between world-space and screen-space coordinates.
  */
class Camera2D {
	public: 
		/**
		 * @class Camera2D
		 * @brief A 2D camera supporting translation, rotation, zoom, and projection.
		 *
		 * Provides functions to configure orthographic projection, viewport scaling,
		 * and to retrieve combined matrices for rendering. Also enables converting
		 * between world-space and screen-space coordinates.
		 */
		Camera2D(float screenwidth, float screenheight);

		// Projection setup
		/**
		 * @brief Sets orthographic projection parameters.
		 * @param left Left boundary in world units.
		 * @param right Right boundary in world units.
		 * @param bottom Bottom boundary in world units.
		 * @param top Top boundary in world units.
		 */
		void SetOrtho(float left, float right, float bottom, float top);
		/**
		 * @brief Sets the viewport transformation.
		 * @param x X offset of the viewport.
		 * @param y Y offset of the viewport.
		 * @param width Viewport width.
		 * @param height Viewport height.
		 */
		void SetViewport(int x, int y, int width, int height);

		//cam controls
		/**
		 * @brief Sets the position of the camera in world space.
		 * @param pos The new camera position.
		 */
		void setPosition(const Vector2& pos);

		/**
		 * @brief Sets the rotation of the camera.
		 * @param radians Rotation angle in radians.
		 */
		void setRotation(float radians);

		/**
		 * @brief Sets the zoom level of the camera.
		 * @param zoom Zoom multiplier (e.g., 1.0 is default, >1.0 zooms in).
		 */
		void setZoom(float zoom);

		/**
		 * @brief Centers the camera on a target point with optional world bounds.
		 * @param target    Target point to center on.
		 * @param worldMin  Minimum corner of the world bounds (use {0,0} for no clamp).
		 * @param worldMax  Maximum corner of the world bounds( use {-1,-1} for no clamp).
		 */
		void lookAt(const Vector2& target, const Vector2& worldMin, const Vector2& worldMax); // follow target to center cam

		// ---------------- Getters ----------------
		Vector2 GetPosition() const { return position; } /// @return Current camera position in world space.
		float   GetRotation() const { return rotation; } /// @return Current rotation in radians.
		float   GetZoom()     const { return zoom; }     /// @return Current zoom factor.

		
		// ---------------- Matrices ----------------
		/**
		* @brief Returns the current projection matrix, updating if dirty.
		* @return Reference to the 3�3 projection matrix.
		*/
		const Matrix3x3& GetProjection();

		/**
		* @brief Returns the current view matrix, updating if dirty.
		* @return Reference to the 3�3 view matrix.
		*/
		const Matrix3x3& GetView();

		const Matrix3x3& GetProjection() const; /// @brief Returns the projection matrix (const version, no update).
		const Matrix3x3& GetView() const;       /// @brief Returns the view matrix (const version, no update).

		/**
		* @brief Computes the combined View-Projection matrix.
		* @return View � Projection matrix.
		*/
		Matrix3x3 GetViewProj();

		/**
		* @brief Computes the full Model-View-Projection matrix.
		* @param model Model transform matrix.
		* @return Projection � View � Model matrix.
		*/
		Matrix3x3 GetMVP(const Matrix3x3& model) const; //build full model mtx proj

		
		/**
		* @brief Retrieves the current visible view size in world units.
		* Calculates the width and height of the region visible through the camera,
		* based on the orthographic projection boundaries and current zoom level.
		* @return Vector2 The width and height of the view in world-space units.
		*/
		Vector2 GetViewSizeWorld() const;
		

		// ---------------- Utilities ----------------
		/**
		* @brief Converts a screen-space point to world-space.
		* @param screen Screen-space coordinates.
		* @return Equivalent world-space position.
		*/
		Vector2 ScreenToWorld(const Vector2& screen);

		/**
		* @brief Converts a world-space point to screen-space.
		* @param world World-space coordinates.
		* @return Equivalent screen-space position.
		*/
		Vector2 WorldToScreen(const Vector2& world);

		/**
		* @brief Rebuilds the view matrix internally if it is dirty.
		*/
		void UpdateView();

		/**
		* @brief Rebuilds the projection matrix internally if it is dirty.
		*/
		void UpdateProjection();

		float viewportWidth = 0.0f;
		float viewportHeight = 0.0f;

	private:
		//float screenWidth, screenHeight;
		Vector2 position;
		float   rotation{ 0.0f };
		float zoom;

		Matrix3x3 view;
		Matrix3x3 projection;
		Matrix3x3 viewport;

		// Rebuild flags
		bool projDirty = true;
		bool viewDirty = true;

		// Cached ortho parameters
		float left, right, bottom, top;

		
};