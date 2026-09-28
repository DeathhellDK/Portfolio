using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class TomNPCScript : MonoBehaviour
{
    public int countercheckforanxity;
    public GameObject TomNPCAnxiety;

    // Update is called once per frame
    void Update()
    {
        TomAIAnxiety();

        TomAnxietyRange();
    }

    public void TomAIAnxiety()
    {
        if (MainMenuScript.TomNPCAnxityLevel >= 0 & MainMenuScript.TomNPCAnxityLevel <= 10)
        {
            TomNPCAnxiety.GetComponent<Renderer>().material.color = Color.blue;
        }

        else if (MainMenuScript.TomNPCAnxityLevel > 10 & MainMenuScript.TomNPCAnxityLevel <= 50)
        {
            TomNPCAnxiety.GetComponent<Renderer>().material.color = Color.green;
        }

        else if (MainMenuScript.TomNPCAnxityLevel >= 51 & MainMenuScript.TomNPCAnxityLevel <= 70)
        {
            TomNPCAnxiety.GetComponent<Renderer>().material.color = Color.yellow;
        }

        else
        {
            TomNPCAnxiety.GetComponent<Renderer>().material.color = Color.red;
        }
    }

    public void TomAnxietyRange()
    {
        if (MainMenuScript.TomNPCAnxityLevel < 0)
        {
            MainMenuScript.TomNPCAnxityLevel = 0;
        }
        else if (MainMenuScript.TomNPCAnxityLevel > 100)
        {
            MainMenuScript.TomNPCAnxityLevel = 100;
        }
    }

    public void OnCollisionExit(Collision collision)
    {
        if (collision.gameObject.CompareTag("Player"))
        {
            countercheckforanxity = 0;
        }
    }

}
