# ORBITAL — Physics & Computational Mechanics Reference

This document outlines the mathematical equations, coordinate systems, units, numerical integrators, and approximations implemented in the **ORBITAL** engine.

---

## 1. Units and Coordinates

- **Mass ($M, m$)**: Kilograms ($\text{kg}$)
- **Distance ($r, R$)**: Meters ($\text{m}$)
- **Time ($t, \Delta t$)**: Seconds ($\text{s}$)
- **Velocity ($v$)**: Meters per second ($\text{m/s}$)
- **Force ($F$)**: Newtons ($\text{N} = \text{kg}\cdot\text{m/s}^2$)
- **Energy ($E, K, U$)**: Joules ($\text{J} = \text{kg}\cdot\text{m}^2/\text{s}^2$)
- **Angles**: Radians internally ($\text{rad}$), degrees ($\text{deg}$) in the user interface.

### Astronomical Constants (CODATA 2018)
- Newton's Gravitational Constant: $G = 6.67430 \times 10^{-11} \text{ m}^3\text{kg}^{-1}\text{s}^{-2}$
- Speed of Light: $c = 299792458 \text{ m/s}$
- Solar Mass ($M_{\odot}$): $1.98847 \times 10^{30} \text{ kg}$
- Solar Radius ($R_{\odot}$): $6.95700 \times 10^8 \text{ m}$
- Earth Mass ($M_{\oplus}$): $5.9722 \times 10^{24} \text{ kg}$
- Earth Radius ($R_{\oplus}$): $6.3710 \times 10^6 \text{ m}$
- Astronomical Unit ($\text{AU}$): $1.495978707 \times 10^{11} \text{ m}$

---

## 2. N-Body Gravitational Dynamics

For a system of $N$ celestial bodies, the gravitational acceleration vector $\mathbf{a}_i$ acting on body $i$ due to all other bodies $j$ is:

$$\mathbf{a}_i = \sum_{j \ne i}^N G \cdot m_j \frac{\mathbf{r}_j - \mathbf{r}_i}{(|\mathbf{r}_j - \mathbf{r}_i|^2 + \epsilon^2)^{3/2}}$$

Where:
- $\mathbf{r}_i, \mathbf{r}_j$ are double-precision position vectors in SI meters.
- $\epsilon$ is the gravitational softening parameter ($\text{m}$), preventing numerical infinities during ultra-close encounters without distorting long-range orbital trajectories.

---

## 3. Numerical Integration Schemes

### 1. Velocity Verlet (Symplectic 2nd-Order) — *Default*
Preserves phase-space volume (symplectic structure), yielding long-term total mechanical energy conservation with bounded drift:

1. $\mathbf{r}(t + \Delta t) = \mathbf{r}(t) + \mathbf{v}(t)\Delta t + \frac{1}{2} \mathbf{a}(t) \Delta t^2$
2. Evaluate new accelerations: $\mathbf{a}(t + \Delta t) = \mathbf{f}(\mathbf{r}(t + \Delta t))$
3. $\mathbf{v}(t + \Delta t) = \mathbf{v}(t) + \frac{1}{2} (\mathbf{a}(t) + \mathbf{a}(t + \Delta t)) \Delta t$

### 2. Semi-Implicit Euler (Symplectic 1st-Order)
1. $\mathbf{v}(t + \Delta t) = \mathbf{v}(t) + \mathbf{a}(t) \Delta t$
2. $\mathbf{r}(t + \Delta t) = \mathbf{r}(t) + \mathbf{v}(t + \Delta t) \Delta t$

### 3. Runge-Kutta 4th Order (RK4)
Higher short-term order of convergence ($O(\Delta t^4)$), suitable for high-speed hyperbolic encounters, though non-symplectic over thousands of orbits.

---

## 4. Conservation Laws and Diagnostics

- **Kinetic Energy**: $K = \sum_i \frac{1}{2} m_i |\mathbf{v}_i|^2$
- **Gravitational Potential Energy**: $U = -\sum_{i < j} \frac{G m_i m_j}{|\mathbf{r}_j - \mathbf{r}_i|}$ (no double counting)
- **Total Mechanical Energy**: $E = K + U$
- **Energy Drift**: $\frac{\Delta E}{E_0} = \frac{E(t) - E(0)}{|E(0)|}$
- **Total Linear Momentum**: $\mathbf{P} = \sum_i m_i \mathbf{v}_i$
- **Total Angular Momentum**: $\mathbf{L} = \sum_i m_i (\mathbf{r}_i \times \mathbf{v}_i)$
- **Center of Mass**: $\mathbf{R}_{\text{com}} = \frac{\sum_i m_i \mathbf{r}_i}{\sum_i m_i}$

---

## 5. Collision Mechanics

### Inelastic Coalescence (Merge Policy)
When bodies touch ($|\mathbf{r}_1 - \mathbf{r}_2| \le R_1 + R_2$):
- **Mass Conservation**: $M_{\text{merged}} = m_1 + m_2$
- **Linear Momentum Conservation**: $\mathbf{v}_{\text{merged}} = \frac{m_1 \mathbf{v}_1 + m_2 \mathbf{v}_2}{m_1 + m_2}$
- **Center of Mass Placement**: $\mathbf{r}_{\text{merged}} = \frac{m_1 \mathbf{r}_1 + m_2 \mathbf{r}_2}{m_1 + m_2}$
- **Volume Conservation**: $R_{\text{merged}} = (R_1^3 + R_2^3)^{1/3}$
- **Impact Energy Released**: $E_{\text{coll}} = \frac{1}{2} \left(\frac{m_1 m_2}{m_1 + m_2}\right) |\mathbf{v}_1 - \mathbf{v}_2|^2$

---

## 6. General Relativity & Black Holes

### Schwarzschild Metric Key Radii
- **Gravitational Radius**: $r_g = \frac{G M}{c^2}$
- **Schwarzschild Radius (Event Horizon)**: $r_s = \frac{2 G M}{c^2}$
- **Photon Sphere**: $r_{\text{ph}} = 1.5 r_s = \frac{3 G M}{c^2}$
- **Innermost Stable Circular Orbit (ISCO)**: $r_{\text{ISCO}} = 3.0 r_s = \frac{6 G M}{c^2}$

### Relativistic Effects
- **Gravitational Time Dilation**:
  $$\frac{d\tau}{dt} = \sqrt{1 - \frac{r_s}{r}}$$
- **Gravitational Redshift**:
  $$z = \frac{1}{\sqrt{1 - r_s / r}} - 1$$
- **1PN Post-Newtonian Periapsis Advance**:
  $$\Delta \mathbf{a}_{\text{1PN}} = \frac{G M}{c^2 r^3} \left[ \left(\frac{4GM}{r} - v^2\right) \mathbf{r} + 4 (\mathbf{r} \cdot \mathbf{v}) \mathbf{v} \right]$$
