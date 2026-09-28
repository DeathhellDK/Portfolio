using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class AnxityPopUpText : MonoBehaviour
{
    public float CDTimer = 2f;
    // Start is called before the first frame update
    void Start()
    {
        Destroy(gameObject, CDTimer);
    }
}
