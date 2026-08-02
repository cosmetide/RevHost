namespace Minecraft.Server.FourKit.Util;

/// <summary>
/// Represents a mutable vector. Because the components of Vectors are mutable,
/// storing Vectors long term may be dangerous if passing code modifies the
/// Vector later. If you want to keep around a Vector, it may be wise to call
/// <see cref="clone"/> in order to get a copy.
/// </summary>
public class Vector
{
    private static readonly double EPSILON = 0.000001;
    private static readonly Random _random = new();

    protected double x;
    protected double y;
    protected double z;

    public Vector()
    {
        x = 0;
        y = 0;
        z = 0;
    }

    /// <summary>
    /// Construct the vector with provided integer components.
    /// </summary>
    /// <param name="x">X component.</param>
    /// <param name="y">Y component.</param>
    /// <param name="z">Z component.</param>
    public Vector(int x, int y, int z)
    {
        this.x = x;
        this.y = y;
        this.z = z;
    }

    /// <summary>
    /// Construct the vector with provided double components.
    /// </summary>
    /// <param name="x">X component.</param>
    /// <param name="y">Y component.</param>
    /// <param name="z">Z component.</param>
    public Vector(double x, double y, double z)
    {
        this.x = x;
        this.y = y;
        this.z = z;
    }

    /// <summary>
    /// Construct the vector with provided float components.
    /// </summary>
    /// <param name="x">X component.</param>
    /// <param name="y">Y component.</param>
    /// <param name="z">Z component.</param>
    public Vector(float x, float y, float z)
    {
        this.x = x;
        this.y = y;
        this.z = z;
    }

    /// <summary>
    /// Adds a vector to this one.
    /// </summary>
    /// <param name="vec">The other vector.</param>
    /// <returns>The same vector.</returns>
    public Vector add(Vector vec)
    {
        x += vec.x;
        y += vec.y;
        z += vec.z;
        return this;
    }

    /// <summary>
    /// Subtracts a vector from this one.
    /// </summary>
    /// <param name="vec">The other vector.</param>
    /// <returns>The same vector.</returns>
    public Vector subtract(Vector vec)
    {
        x -= vec.x;
        y -= vec.y;
        z -= vec.z;
        return this;
    }

    /// <summary>
    /// Multiplies the vector by another.
    /// </summary>
    /// <param name="vec">The other vector.</param>
    /// <returns>The same vector.</returns>
    public Vector multiply(Vector vec)
    {
        x *= vec.x;
        y *= vec.y;
        z *= vec.z;
        return this;
    }

    /// <summary>
    /// Divides the vector by another.
    /// </summary>
    /// <param name="vec">The other vector.</param>
    /// <returns>The same vector.</returns>
    public Vector divide(Vector vec)
    {
        x /= vec.x;
        y /= vec.y;
        z /= vec.z;
        return this;
    }

    /// <summary>
    /// Copies another vector.
    /// </summary>
    /// <param name="vec">The other vector.</param>
    /// <returns>The same vector.</returns>
    public Vector copy(Vector vec)
    {
        x = vec.x;
        y = vec.y;
        z = vec.z;
        return this;
    }

    /// <summary>
    /// Gets the magnitude of the vector, defined as sqrt(x^2+y^2+z^2).
    /// The value of this method is not cached and uses a costly square-root
    /// function, so do not repeatedly call this method to get the vector's
    /// magnitude. NaN will be returned if the inner result of the sqrt()
    /// function overflows, which will be caused if the length is too long.
    /// </summary>
    /// <returns>The magnitude.</returns>
    public double length()
    {
        return Math.Sqrt(x * x + y * y + z * z);
    }

    /// <summary>
    /// Gets the magnitude of the vector squared.
    /// </summary>
    /// <returns>The magnitude squared.</returns>
    public double lengthSquared()
    {
        return x * x + y * y + z * z;
    }

    /// <summary>
    /// Get the distance between this vector and another. The value of this
    /// method is not cached and uses a costly square-root function, so do not
    /// repeatedly call this method to get the vector's magnitude. NaN will be
    /// returned if the inner result of the sqrt() function overflows, which
    /// will be caused if the distance is too long.
    /// </summary>
    /// <param name="o">The other vector.</param>
    /// <returns>The distance.</returns>
    public double distance(Vector o)
    {
        return Math.Sqrt(distanceSquared(o));
    }

    /// <summary>
    /// Get the squared distance between this vector and another.
    /// </summary>
    /// <param name="o">The other vector.</param>
    /// <returns>The distance squared.</returns>
    public double distanceSquared(Vector o)
    {
        double dx = x - o.x;
        double dy = y - o.y;
        double dz = z - o.z;
        return dx * dx + dy * dy + dz * dz;
    }

    /// <summary>
    /// Gets the angle between this vector and another in radians.
    /// </summary>
    /// <param name="other">The other vector.</param>
    /// <returns>Angle in radians.</returns>
    public float angle(Vector other)
    {
        double dot = this.dot(other) / (length() * other.length());
        return (float)Math.Acos(dot);
    }

    /// <summary>
    /// Sets this vector to the midpoint between this vector and another.
    /// </summary>
    /// <param name="other">The other vector.</param>
    /// <returns>This same vector (now a midpoint).</returns>
    public Vector midpoint(Vector other)
    {
        x = (x + other.x) / 2.0;
        y = (y + other.y) / 2.0;
        z = (z + other.z) / 2.0;
        return this;
    }

    /// <summary>
    /// Gets a new midpoint vector between this vector and another.
    /// </summary>
    /// <param name="other">The other vector.</param>
    /// <returns>A new midpoint vector.</returns>
    public Vector getMidpoint(Vector other)
    {
        double mx = (x + other.x) / 2.0;
        double my = (y + other.y) / 2.0;
        double mz = (z + other.z) / 2.0;
        return new Vector(mx, my, mz);
    }

    /// <summary>
    /// Performs scalar multiplication, multiplying all components with a scalar.
    /// </summary>
    /// <param name="m">The factor.</param>
    /// <returns>The same vector.</returns>
    public Vector multiply(int m)
    {
        x *= m;
        y *= m;
        z *= m;
        return this;
    }

    /// <summary>
    /// Performs scalar multiplication, multiplying all components with a scalar.
    /// </summary>
    /// <param name="m">The factor.</param>
    /// <returns>The same vector.</returns>
    public Vector multiply(double m)
    {
        x *= m;
        y *= m;
        z *= m;
        return this;
    }

    /// <summary>
    /// Performs scalar multiplication, multiplying all components with a scalar.
    /// </summary>
    /// <param name="m">The factor.</param>
    /// <returns>The same vector.</returns>
    public Vector multiply(float m)
    {
        x *= m;
        y *= m;
        z *= m;
        return this;
    }

    /// <summary>
    /// Calculates the dot product of this vector with another. The dot product
    /// is defined as x1*x2+y1*y2+z1*z2. The returned value is a scalar.
    /// </summary>
    /// <param name="other">The other vector.</param>
    /// <returns>Dot product.</returns>
    public double dot(Vector other)
    {
        return x * other.x + y * other.y + z * other.z;
    }

    /// <summary>
    /// Calculates the cross product of this vector with another.
    /// The cross product is defined as:
    /// <code>
    /// x = y1 * z2 - y2 * z1
    /// y = z1 * x2 - z2 * x1
    /// z = x1 * y2 - x2 * y1
    /// </code>
    /// </summary>
    /// <param name="o">The other vector.</param>
    /// <returns>The same vector.</returns>
    public Vector crossProduct(Vector o)
    {
        double newX = y * o.z - o.y * z;
        double newY = z * o.x - o.z * x;
        double newZ = x * o.y - o.x * y;
        x = newX;
        y = newY;
        z = newZ;
        return this;
    }

    /// <summary>
    /// Converts this vector to a unit vector (a vector with length of 1).
    /// </summary>
    /// <returns>The same vector.</returns>
    public Vector normalize()
    {
        double mag = length();
        x /= mag;
        y /= mag;
        z /= mag;
        return this;
    }

    /// <summary>
    /// Zero this vector's components.
    /// </summary>
    /// <returns>The same vector.</returns>
    public Vector zero()
    {
        x = 0;
        y = 0;
        z = 0;
        return this;
    }

    /// <summary>
    /// Returns whether this vector is in an axis-aligned bounding box.
    /// The minimum and maximum vectors given must be truly the minimum and
    /// maximum X, Y and Z components.
    /// </summary>
    /// <param name="min">Minimum vector.</param>
    /// <param name="max">Maximum vector.</param>
    /// <returns>Whether this vector is in the AABB.</returns>
    public bool isInAABB(Vector min, Vector max)
    {
        return x >= min.x && x <= max.x &&
               y >= min.y && y <= max.y &&
               z >= min.z && z <= max.z;
    }

    /// <summary>
    /// Returns whether this vector is within a sphere.
    /// </summary>
    /// <param name="origin">Sphere origin.</param>
    /// <param name="radius">Sphere radius.</param>
    /// <returns>Whether this vector is in the sphere.</returns>
    public bool isInSphere(Vector origin, double radius)
    {
        return distanceSquared(origin) <= radius * radius;
    }

    /// <summary>
    /// Gets the X component.
    /// </summary>
    /// <returns>The X component.</returns>
    public double getX()
    {
        return x;
    }

    /// <summary>
    /// Gets the floored value of the X component, indicating the block that
    /// this vector is contained with.
    /// </summary>
    /// <returns>Block X.</returns>
    public int getBlockX()
    {
        return (int)Math.Floor(x);
    }

    /// <summary>
    /// Gets the Y component.
    /// </summary>
    /// <returns>The Y component.</returns>
    public double getY()
    {
        return y;
    }

    /// <summary>
    /// Gets the floored value of the Y component, indicating the block that
    /// this vector is contained with.
    /// </summary>
    /// <returns>Block Y.</returns>
    public int getBlockY()
    {
        return (int)Math.Floor(y);
    }

    /// <summary>
    /// Gets the Z component.
    /// </summary>
    /// <returns>The Z component.</returns>
    public double getZ()
    {
        return z;
    }

    /// <summary>
    /// Gets the floored value of the Z component, indicating the block that
    /// this vector is contained with.
    /// </summary>
    /// <returns>Block Z.</returns>
    public int getBlockZ()
    {
        return (int)Math.Floor(z);
    }

    /// <summary>
    /// Set the X component.
    /// </summary>
    /// <param name="x">The new X component.</param>
    /// <returns>This vector.</returns>
    public Vector setX(int x)
    {
        this.x = x;
        return this;
    }

    /// <summary>
    /// Set the X component.
    /// </summary>
    /// <param name="x">The new X component.</param>
    /// <returns>This vector.</returns>
    public Vector setX(double x)
    {
        this.x = x;
        return this;
    }

    /// <summary>
    /// Set the X component.
    /// </summary>
    /// <param name="x">The new X component.</param>
    /// <returns>This vector.</returns>
    public Vector setX(float x)
    {
        this.x = x;
        return this;
    }

    /// <summary>
    /// Set the Y component.
    /// </summary>
    /// <param name="y">The new Y component.</param>
    /// <returns>This vector.</returns>
    public Vector setY(int y)
    {
        this.y = y;
        return this;
    }

    /// <summary>
    /// Set the Y component.
    /// </summary>
    /// <param name="y">The new Y component.</param>
    /// <returns>This vector.</returns>
    public Vector setY(double y)
    {
        this.y = y;
        return this;
    }

    /// <summary>
    /// Set the Y component.
    /// </summary>
    /// <param name="y">The new Y component.</param>
    /// <returns>This vector.</returns>
    public Vector setY(float y)
    {
        this.y = y;
        return this;
    }

    /// <summary>
    /// Set the Z component.
    /// </summary>
    /// <param name="z">The new Z component.</param>
    /// <returns>This vector.</returns>
    public Vector setZ(int z)
    {
        this.z = z;
        return this;
    }

    /// <summary>
    /// Set the Z component.
    /// </summary>
    /// <param name="z">The new Z component.</param>
    /// <returns>This vector.</returns>
    public Vector setZ(double z)
    {
        this.z = z;
        return this;
    }

    /// <summary>
    /// Set the Z component.
    /// </summary>
    /// <param name="z">The new Z component.</param>
    /// <returns>This vector.</returns>
    public Vector setZ(float z)
    {
        this.z = z;
        return this;
    }

    /// <summary>
    /// Get a new vector.
    /// </summary>
    /// <returns>A clone of this vector.</returns>
    public Vector clone()
    {
        return new Vector(x, y, z);
    }

    /// <summary>
    /// Gets a Location version of this vector with yaw and pitch being 0.
    /// </summary>
    /// <param name="world">The world to link the location to.</param>
    /// <returns>The location.</returns>
    public Location toLocation(World world)
    {
        return new Location(world, x, y, z);
    }

    /// <summary>
    /// Gets a Location version of this vector.
    /// </summary>
    /// <param name="world">The world to link the location to.</param>
    /// <param name="yaw">The desired yaw.</param>
    /// <param name="pitch">The desired pitch.</param>
    /// <returns>The location.</returns>
    public Location toLocation(World world, float yaw, float pitch)
    {
        return new Location(world, x, y, z, yaw, pitch);
    }

    /// <summary>
    /// Get the threshold used for equals().
    /// </summary>
    /// <returns>The epsilon.</returns>
    public static double getEpsilon()
    {
        return EPSILON;
    }

    /// <summary>
    /// Gets the minimum components of two vectors.
    /// </summary>
    /// <param name="v1">The first vector.</param>
    /// <param name="v2">The second vector.</param>
    /// <returns>Minimum.</returns>
    public static Vector getMinimum(Vector v1, Vector v2)
    {
        return new Vector(Math.Min(v1.x, v2.x), Math.Min(v1.y, v2.y), Math.Min(v1.z, v2.z));
    }

    /// <summary>
    /// Gets the maximum components of two vectors.
    /// </summary>
    /// <param name="v1">The first vector.</param>
    /// <param name="v2">The second vector.</param>
    /// <returns>Maximum.</returns>
    public static Vector getMaximum(Vector v1, Vector v2)
    {
        return new Vector(Math.Max(v1.x, v2.x), Math.Max(v1.y, v2.y), Math.Max(v1.z, v2.z));
    }

    /// <summary>
    /// Gets a random vector with components having a random value between 0 and 1.
    /// </summary>
    /// <returns>A random vector.</returns>
    public static Vector getRandom()
    {
        return new Vector(_random.NextDouble(), _random.NextDouble(), _random.NextDouble());
    }

    /// <inheritdoc/>
    public override bool Equals(object? obj)
    {
        if (obj is not Vector other) return false;
        return Math.Abs(x - other.x) < EPSILON &&
               Math.Abs(y - other.y) < EPSILON &&
               Math.Abs(z - other.z) < EPSILON;
    }

    /// <inheritdoc/>
    public override string ToString()
    {
        return $"{x},{y},{z}";
    }
}
