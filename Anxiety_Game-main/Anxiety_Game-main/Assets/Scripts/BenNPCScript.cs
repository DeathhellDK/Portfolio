using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class BenNPCScript : MonoBehaviour
{
    public int countercheckforanxity;

    public GameObject BenNPCAnxiety;

    // Update is called once per frame
    void Update()
    {
        BenAIAnxiety();

        BenAnxietyRange();
    }

    public void BenAIAnxiety()
    {
        if (MainMenuScript.BenNPCAnxityLevel >= 0 & MainMenuScript.BenNPCAnxityLevel <= 10)
        {
            BenNPCAnxiety.GetComponent<Renderer>().material.color = Color.blue;
        }

        else if (MainMenuScript.BenNPCAnxityLevel > 10 & MainMenuScript.BenNPCAnxityLevel <= 50)
        {
            BenNPCAnxiety.GetComponent<Renderer>().material.color = Color.green;
        }

        else if (MainMenuScript.BenNPCAnxityLevel >= 51 & MainMenuScript.BenNPCAnxityLevel <= 70)
        {
            BenNPCAnxiety.GetComponent<Renderer>().material.color = Color.yellow;
        }

        else
        {
            BenNPCAnxiety.GetComponent<Renderer>().material.color = Color.red;
        }
    }

    public void BenAnxietyRange()
    {
        if(MainMenuScript.BenNPCAnxityLevel <= 0)
        {
            MainMenuScript.BenNPCAnxityLevel = 0;
        }
        else if(MainMenuScript.BenNPCAnxityLevel > 100)
        {
            MainMenuScript.BenNPCAnxityLevel = 100;
        }
    }

    public void OnCollisionExit(Collision collision)
    {
        if(collision.gameObject.CompareTag("Player"))
        {
            countercheckforanxity = 0;
        }
    }

}
