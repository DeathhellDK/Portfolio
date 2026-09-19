    /**
     * @file    camera2d.cpp
     * @author  Jethro
     * @email     w.kyele, sung.h
     * @co-author Woh Kye Le
     * @date    2025-09-29
     *
     * @brief   Implements the Camera2D class for 2D rendering.
     *
     * Contains logic for orthographic projection, view matrix updates,
     * and utilities for coordinate conversions. Provides model-view-projection
     * building for rendering pipelines.
     *
     * @version 1.0
     */
    #include "Graphics/camera2d.h"
    #include <cmath>
    #include <algorithm>

    /**
    * @brief Constructs a Camera2D with screen dimensions.
    *
    * Initializes orthographic projection and viewport transform
    * using the provided width and height. Forces an initial
    * projection and view update to avoid uninitialized matrices.
    *
    * @param width Screen width in pixels.
    * @param height Screen height in pixels.
    */
    Camera2D::Camera2D(float width, float height) : position(0, 0), rotation(0.0f), zoom(1.0f) {
        SetOrtho(0.0f, width, 0.0f, height);
        SetViewport(0, 0, static_cast<int>(width), static_cast<int>(height));
        // Force initial update so projection/view are not zero
        UpdateProjection();
        UpdateView();
    }
    /**
     * @brief Sets the orthographic projection boundaries.
     * @param l Left boundary.
     * @param r Right boundary.
     * @param b Bottom boundary.
     * @param t Top boundary.
     */
    void Camera2D::SetOrtho(float l, float r, float b, float t) {
        left = l; right = r; bottom = b; top = t;
        projDirty = true;
    }

    /**
    * @brief Configures the viewport transformation matrix.
    *
    * The viewport maps normalized device coordinates (−1..1) to
    * pixel coordinates. This also caches the viewport’s pixel width
    * and height for later camera operations.
    *
    * @param x X offset in pixels.
    * @param y Y offset in pixels.
    * @param w Viewport width in pixels.
    * @param h Viewport height in pixels.
    */
    void Camera2D::SetViewport(int x, int y, int w, int h) {
        float fx = static_cast<float>(x);
        float fy = static_cast<float>(y);
        float fw = static_cast<float>(w);
        float fh = static_cast<float>(h);
        viewport = Matrix3x3::BuildTranslation(fx + fw / 2.0f, fy + fh / 2.0f)
            * Matrix3x3::BuildScaling(fw / 2.0f, fh / 2.0f);
        viewportWidth = fw;
        viewportHeight = fh;
    }

    /**
    * @brief Sets the camera position and marks the view matrix dirty.
    * @param pos New world-space position.
    */
    void Camera2D::setPosition(const Vector2& pos) {
        position = pos;
        viewDirty = true;
    }

    /**
    * @brief Sets the camera rotation (in radians) and marks the view matrix dirty.
    * @param radians Rotation angle in radians.
    */
    void Camera2D::setRotation(float radians) {
        rotation = radians;
        viewDirty = true;
    }

    /**
    * @brief Sets the zoom level and marks the view matrix dirty.
    * @param z New zoom factor.
    */
    void Camera2D::setZoom(float z) {
        zoom = z;
        viewDirty = true;
    }

    /**
    * @brief Returns the current projection matrix, updating if dirty.
    * @return Reference to the projection matrix.
    */
    const Matrix3x3& Camera2D::GetProjection() {
        if (projDirty) UpdateProjection();
        return projection;
    }

    /**
    * @brief Returns the current view matrix, updating if dirty.
    * @return Reference to the view matrix.
    */
    const Matrix3x3& Camera2D::GetView() {
        if (viewDirty) UpdateView();
        return view;
    }

    /**
    * @brief Returns the last computed projection matrix (const).
    */
    const Matrix3x3& Camera2D::GetProjection() const {
        return projection;
    }

    /**
    * @brief Returns the last computed view matrix (const).
    */
    const Matrix3x3& Camera2D::GetView() const {
        return view;
    }

    /**
    * @brief Computes the combined View-Projection matrix.
    * @return Projection × View matrix.
    */
    Matrix3x3 Camera2D::GetViewProj() {
        return GetProjection() * GetView();
    }

    /**
    * @brief Builds a full Model-View-Projection matrix.
    * @param model The model matrix to apply.
    * @return Projection × View × Model matrix.
    */
    Matrix3x3 Camera2D::GetMVP(const Matrix3x3& model) const {
        return GetProjection() * GetView() * model;
    }

    Vector2 Camera2D::GetViewSizeWorld() const {
        return { (right - left) / zoom, (top - bottom) / zoom };
    }

    /**
     * @brief Updates the orthographic projection matrix.
     *
     * Builds a projection matrix from left/right/bottom/top parameters.
     * Handles degenerate cases (when width or height ~ 0) by clamping.
     */
    void Camera2D::UpdateProjection() {
        //projection = Matrix3x3::Ortho(left, right, bottom, top);
        float l = left;
        float r = right;
        float b = bottom;
        float t = top;

        if (fabs(r - l) < 1e-6f) r = l + 1.0f;
        if (fabs(t - b) < 1e-6f) t = b + 1.0f;

        projection = Matrix3x3::Identity();
        projection(0, 0) = 2.0f / (r - l);
        projection(1, 1) = 2.0f / (t - b);   // scale Y (positive if you want Y-up)
        projection(2, 0) = -(r + l) / (r - l); // tx in column 2, row 0
        projection(2, 1) = -(t + b) / (t - b); // ty in column 2, row 1
        projDirty = false;
    }

    /**
     * @brief Updates the view matrix from position, rotation, and zoom.
     *
     * Applies translation, rotation, and scaling (inverse zoom).
     */
    void Camera2D::UpdateView() {
        Matrix3x3 T = Matrix3x3::BuildTranslation(-position.x, -position.y);
        Matrix3x3 R = Matrix3x3::BuildRotation(-rotation);
        Matrix3x3 S = Matrix3x3::BuildScaling(1.0f / zoom, 1.0f / zoom);
        //view = R * S * T;
        view = S * R * T;
        viewDirty = false;
    }

    /**
     * @brief Converts screen-space coordinates to world-space.
     *
     * Multiplies screen coordinates by the inverse of (ViewProj * Viewport).
     *
     * @param screen 2D screen coordinates.
     * @return World-space coordinates.
     */
    Vector2 Camera2D::ScreenToWorld(const Vector2& screen) {
        Matrix3x3 vp = viewport * GetViewProj();
        Matrix3x3 inv = vp.Inverse();
        return inv * screen;
    }

    /**
     * @brief Converts world-space coordinates to screen-space.
     *
     * Multiplies world coordinates by (Viewport * ViewProj).
     *
     * @param world 2D world coordinates.
     * @return Screen-space coordinates.
     */
    Vector2 Camera2D::WorldToScreen(const Vector2& world) {
        Matrix3x3 vp = viewport * GetViewProj();
        return vp * world; // uses Matrix3x3::operator*(Vector2)
    }

    /**
     * @brief Center the camera on a target point with optional world bounds, the camera is positioned such that 
     *        the target appears in the centre of the screen given the current zoom and viewport size , if worldMin and 
     *        worldMax is specified, the camera is clamped such that visible region never leave those bound. howevere 
     *        when the world is smaller than the view, the camera is pinned in such a way that the world stayed 
     *        centered 
     *
     * @param target point where you want centered on screen
     * @param worldMin  Minimum (x,y) corner of the world AABB
     * @param worldMax  Maximum (x,y) corner of the world AABB 
     * 
     * @par Example
     * // Center camera on the player while clamping to a 0..2000 x 0..1200 world:
     * cam.lookAt(playerCenter, {0.f, 0.f}, {2000.f, 1200.f});
     *
     * // Free center (no clamping):
     * cam.lookAt(playerCenter, {0.f, 0.f}, {-1.f, -1.f}); // invalid bounds -> unclamped
     * 
     */
    void Camera2D::lookAt(const Vector2& target, const Vector2& worldMin, const Vector2& worldMax) {

        // uses orthographic world size
        const float viewW_world = (right - left) / zoom;
        const float viewH_world = (top - bottom) / zoom;
        const Vector2 half(viewW_world * 0.5f, viewH_world * 0.5f);

        // position such that target ends up centered
        Vector2 desiredPos = target - half;

        const bool hasWorldBounds = (worldMax.x > worldMin.x) && (worldMax.y > worldMin.y);
        if (hasWorldBounds)
        {
            // if the world is smaller than the view, pin to the world's center
            const float worldW = worldMax.x - worldMin.x;
            const float worldH = worldMax.y - worldMin.y;
            const float viewW = half.x * 2.0f;
            const float viewH = half.y * 2.0f;
            constexpr float eps = 1e-5f;


            // If the world is strictly smaller than the view, pin to the world's center.
            // If equal or larger, clamp normally so the camera can move.
            if (worldW < viewW - eps) {
                desiredPos.x = worldMin.x + 0.5f * (worldW - viewW);
            }
            else {
                const float maxX = worldMax.x - viewW;
                desiredPos.x = std::clamp(desiredPos.x, worldMin.x, maxX);
            }

            if (worldH < viewH - eps) {
                desiredPos.y = worldMin.y + 0.5f * (worldH - viewH);
            }
            else {
                const float maxY = worldMax.y - viewH;
                desiredPos.y = std::clamp(desiredPos.y, worldMin.y, maxY);
            }
        }

        setPosition(desiredPos);   // marks viewDirty
        UpdateView();

    }

