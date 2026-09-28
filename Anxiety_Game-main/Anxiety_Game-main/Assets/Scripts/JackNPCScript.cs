using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class JackNPCScript : MonoBehaviour
{
    public int countercheckforanxity;
    public GameObject JackNPCAnxiety;


    // Update is called once per frame
    void Update()
    {
        JackAIAnxiety();

        JackAnxietyRange();

    }

    public void JackAIAnxiety()
    {
        if (MainMenuScript.JackNPCAnxityLevel >= 0 & MainMenuScript.JackNPCAnxityLevel <= 10)
        {
            JackNPCAnxiety.GetComponent<Renderer>().material.color = Color.blue;
        }

        else if (MainMenuScript.JackNPCAnxityLevel > 10 & MainMenuScript.JackNPCAnxityLevel <= 50)
        {
            JackNPCAnxiety.GetComponent<Renderer>().material.color = Color.green;
        }

        else if (MainMenuScript.JackNPCAnxityLevel >= 51 & MainMenuScript.JackNPCAnxityLevel <= 70)
        {
            JackNPCAnxiety.GetComponent<Renderer>().material.color = Color.yellow;
        }

        else
        {
            JackNPCAnxiety.GetComponent<Renderer>().material.color = Color.red;
        }
    }

    public void JackAnxietyRange()
    {
        if (MainMenuScript.JackNPCAnxityLevel < 0)
        {
            MainMenuScript.JackNPCAnxityLevel = 0;
        }
        else if (MainMenuScript.JackNPCAnxityLevel > 100)
        {
            MainMenuScript.JackNPCAnxityLevel = 100;
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
