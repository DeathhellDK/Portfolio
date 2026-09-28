using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.SceneManagement;

public class MainMenuScript : MonoBehaviour
{
    public string HowToPlay;
    
    public static float HealthValue;
    public static int NPCClear = 0;

    public static int BenNPCAnxityLevel;
    public static int TomNPCAnxityLevel;
    public static int JackNPCAnxityLevel;

    public static bool tutcheck;
    public static bool tutorialcheck;


    void Start()
    {
        TimeSystem.month = 1;
        TimeSystem.day = 1;
        TimeSystem.year = 2020;
        TimeSystem.hour = 7;

        
        MainMenuScript.NPCClear = 0;

        BenNPCAnxityLevel = 90;
        JackNPCAnxityLevel = 90;
        TomNPCAnxityLevel = 90;

        HealthValue = 1;

        tutcheck = true;
        tutorialcheck = true;
    }

    public void HowToPlayGame()
    {
        SceneManager.LoadScene(HowToPlay);
    }
    public void QuitGame()
    {
        Application.Quit();
    }
}
