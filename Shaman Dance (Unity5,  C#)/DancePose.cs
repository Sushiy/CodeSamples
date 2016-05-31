using UnityEngine;
using System.Collections;

public class DancePose
{
    private const float AllowedAngleDeviation = 30;

    public float right;
    public float left;

    public Vector2 rVec;
    public Vector2 lVec;

    private const float distanceMax = 1f;

    public DancePose(float left, float right)
    {
        this.left = left;
        this.right = right;
    }

    public bool IsPoseHit(float left, float right)
    {
        return IsPoseHit(left,right,false);
    }

    public bool IsPoseHit(float left, float right, bool debug)
    {
        float a = 180 - Mathf.Abs(Mathf.Abs(this.left - left) - 180);
        float b = 180 - Mathf.Abs(Mathf.Abs(this.right - right) - 180);
        //if (debug)
            //Debug.Log(left + "|" + right + "/" + this.left + "|" + this.right + ">>>" + a + "/" + b);
        return (a < AllowedAngleDeviation && b < AllowedAngleDeviation);
    }

    public bool IsPoseHitVec(Vector2 left, Vector2 right)
    {
        //Debug.Log("left" + left.ToString() + " right" + right.ToString());
        return (Vector2.Angle(left, lVec) <= AllowedAngleDeviation && Vector2.Angle(right, rVec) <= AllowedAngleDeviation);
    }
}
