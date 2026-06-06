# Design: Diorama for simulation and robotics (and the Fedora RPM stack)

Status: design / strategy note. No implementation yet. Grounded against the actual
ROS 2 install and the ROS 2 Gem on this machine.

## Honest verdict (read this first)

A 2D *engine* has no robotics-*simulation* story: the simulation is 3D (physics,
sensors, kinematics, URDF), and Diorama touches none of it. Diorama is a 2D *content
layer* riding on O3DE, which is the thing doing robotics. So the story here is real
but **narrow and supporting**, not central. The one-line pitch this document will
defend, and nothing larger:

> A clean way to author and live-stream the inherently-2D content of a 3D
> simulation, some of which the simulated sensors can actually see.

Everything below is scoped to that sentence. The "Limits" section says plainly where
this is not a fit, so the note cannot be read as claiming Diorama is a robotics tool.

## Context (grounded)

- O3DE's next release theme is simulation and robotics focused.
- A Fedora-native robotics stack is becoming real, not hypothetical:
  - ROS 2 **Jazzy** is packaged as Fedora RPMs (the `hellaenergy/ros2` COPR, source
    at `github.com/nickschuetz/ros2-rpm`): **Fedora 44+ and CentOS Stream 10**,
    **x86_64 and aarch64**, via `ros-jazzy-ros-base` / `ros-jazzy-ros-desktop`.
    Confirmed installed here under `/opt/ros/jazzy` (`ros-jazzy-*-1.fc44`).
  - That repo is explicitly **development-only**; Open Robotics takes on **official**
    Fedora support starting with **Lyrical Luth (the 2026 LTS)**.
  - The COPR already ships **`gazebo_msgs` specifically for the O3DE ROS 2 Gem**, so
    O3DE integration is already part of that packaging scope.
- The O3DE robotics stack (the **ROS 2 Gem**, Apache-2.0) lives in `o3de-extras`, not
  the core SDK.

So a fully open, `dnf`-installable robotics simulation stack (ROS 2 + O3DE) is
buildable now and goes official-grade with Lyrical. Diorama is an optional content
layer on top, not part of that critical path.

## The one fit that is genuinely strong: live 2D data

Some robotics data is *intrinsically* 2D, so a 2D representation is natural rather
than a costume. The strongest case is the occupancy grid / costmap:

- **`nav_msgs/OccupancyGrid` to a live ground tilemap.** An occupancy grid is a 2D
  array; mobile-robot navigation happens on a plane. This is more than a convenience:
  O3DE has no first-class way to render a **dynamic 2D data grid on a surface that
  updates per-frame from a stream**, with per-cell values. "Just texture a plane"
  covers a *static* image only; a live costmap that changes as the robot explores
  needs a dynamic, per-cell, scriptable grid, which is exactly what the Diorama
  tilemap plus its `SetGridSize`/`Fill`/`SetTile` bus already is. This is the lead.
- **`nav_msgs/Path` to line-quad sprites** and **`sensor_msgs/LaserScan` (downsampled
  `PointCloud2`) to ground sprites** are the same idea for other intrinsically-2D,
  live streams: a planned path or a scan ring drawn on the floor, updating each frame.

Verified present in `/opt/ros/jazzy/include`: `rclcpp`, `nav_msgs`, `geometry_msgs`,
`tf2_ros`, `sensor_msgs` (used via `tf2_ros` to place these in the right frame).

## Supporting fits: flat content the sim renders and sensors can see

Weaker than the live-grid case, but legitimate because the content is flat by nature
*and lives in the simulated world* (an external viewer cannot put content into the
world the robot perceives):

- **Fiducials (AprilTag / ArUco) as world sprites.** Flat textures in reality; a
  simulated camera sensor can actually see them, so they are real input to a
  perception, calibration, or SLAM test, not just a human overlay.
- **Floor markings and zones** (lanes, charging pads, no-go regions) for AMR /
  warehouse sims: visible content that, paired with the planned 2D trigger zones, can
  also be functional.
- **The sensor-visible subset of `visualization_msgs/MarkerArray`.** Markers meant for
  a *human* are already served by rviz/Foxglove; markers that must appear in a
  *rendered camera feed* share the fiducial justification. This is gated:
  `visualization_msgs` is **not** in the installed/minimal COPR set today, so it needs
  either `ros-jazzy-visualization-msgs` added to the COPR (small, permissive BSD) or a
  Diorama-native marker layer first.

## Limits, and where this is NOT a fit

- **Not a robotics simulator.** No physics, no sensor models, no kinematics. PhysX and
  the ROS 2 Gem own all of that.
- **Convenience, not enabler, for static content.** A static map or a fixed marker can
  already be a textured plane or a decal in O3DE. Diorama's genuine edge is the
  *dynamic, live-streamed, per-cell* case, not static imagery.
- **Human-facing debug markers are already solved** by rviz/Foxglove/web dashboards;
  only the sensor-visible subset is distinctive here.
- **Not a 2D physics sim either.** 2D research sims (multi-agent, RL gridworlds) exist,
  but use lightweight Python stacks; O3DE is the wrong weight class and Diorama's
  planned collision is scenario-trigger-grade, not a physics solver.
- **Do not force a grand narrative.** The engine's robotics story should lead with the
  real robotics tech (ROS 2 Gem, sensors, physics). Diorama is at most a small,
  optional content layer, and Diorama itself is still pre-1.0 (beta).

## The `diorama-ros2` bridge (sketch)

A **separate optional gem/module**, so Diorama core stays ROS-free:

- Rides the ROS 2 Gem's existing node rather than opening its own ROS context:
  `ROS2Interface::Get()->GetNode()` returns a `std::shared_ptr<rclcpp::Node>`
  (`o3de-extras/Gems/ROS2/Code/Include/ROS2/ROS2Bus.h:38`; the gem's own clock and
  spawner subscribe this way). The bridge creates subscriptions on that node and
  drives the Diorama buses.
- CMake follows the ROS 2 Gem's pattern: `find_package(ROS2 MODULE)` +
  `target_depends_on_ros2_packages(rclcpp nav_msgs sensor_msgs tf2_ros)` (add
  `visualization_msgs` only if/when packaged).
- License: Apache-2.0 (matches the ROS 2 Gem; Diorama core is Apache-2.0 OR MIT).

## Packaging chain (all RPM, all open)

`ros-jazzy-ros-base` (COPR; official Lyrical later) -> `o3de` RPM SDK (COPR) ->
`diorama` RPM (one gem dep `Atom_RPI`, **no bundled third-party**, verified) ->
`diorama-ros2` subpackage (deps: the ROS 2 Gem + system `ros-base` [+
`visualization-msgs` if used]). Target matrix mirrors the COPR: Fedora 44+ / CentOS
Stream 10, x86_64 + aarch64.

License hygiene is Fedora-clean by construction: Diorama is `Apache-2.0 OR MIT` with
SPDX headers and original/open-licensed assets only, and links nothing it would need
to unbundle.

## Relationship to rviz2 (complementary, not a substitute)

`rviz2` is the standard ROS visualizer: an **external** operator/debug window onto
live topic data that works against real robots, not just sims. It is not packaged on
Fedora *yet* (upstream `ros2/rviz#1708` Ogre / CMake 4.x, `ros2/rviz#1730` Assimp /
GCC), but that gap is **temporary and closeable** (rviz2 can be packaged when needed,
and Lyrical will carry it), so Diorama's value does not rest on it. The two are
complementary: rviz2 draws data *about* the world in its own window; Diorama draws
*inside the simulated world itself*, where the sim camera and sensors can see it.

## Honest demo scope

`nav2_*` is deliberately out of scope in the COPR (deferred to Lyrical). So the demo
today is **not** a full Nav2 stack from `dnf`. Instead a small `rclcpp` node
(buildable on `ros-base` alone) publishes a synthetic or recorded `OccupancyGrid` +
`Path`, and Diorama visualizes it inside O3DE. Everything is `dnf`-installable; only
the *data source* is a stand-in. Swap in the real stack when Lyrical + nav2 land, with
no change to the bridge.

## Open decisions

- **Bridge home**: `diorama-ros2` in the Diorama repo (default; keeps core ROS-free)
  vs. proposing it to **o3de-extras** alongside the ROS 2 Gem (needs explicit go-ahead
  before anything is filed there).
- **`visualization_msgs`**: request it in the COPR to enable the sensor-visible marker
  subset, or ship a Diorama-native marker layer first.

## Next steps (no monitor needed for the first)

1. A focused `diorama-ros2` bridge design doc (subscription-to-bus mapping,
   node-sharing, CMake/subpackage layout), grounded against the ROS 2 Gem source.
2. Prototype the live `OccupancyGrid` subscriber -> tilemap against the installed
   Jazzy + ROS 2 Gem; verify a published grid renders and updates as a ground tilemap.
