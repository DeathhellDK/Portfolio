using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.SceneManagement;

public class EndGameSceneManager : MonoBehaviour
{
    public GameObject GoodEnding;
    public GameObject NormalEnding;
    public GameObject BadEnding;

    public string MainMenu;

    // Start is called before the first frame update
    void Start()
    {
        
    }

    // Update is called once per frame
    void Update()
    {
        CheckConditionofPlayerStatues();
    }

    void CheckConditionofPlayerStatues()
    {
        if (MainMenuScript.NPCClear == 3 & MainMenuScript.BenNPCAnxityLevel == 0 & MainMenuScript.TomNPCAnxityLevel == 0 & MainMenuScript.JackNPCAnxityLevel == 0)
        {
            GoodEnding.SetActive(true);
           
        }
        else if(MainMenuScript.NPCClear > 0 & MainMenuScript.NPCClear <= 3 & MainMenuScript.BenNPCAnxityLevel > 0 & MainMenuScript.TomNPCAnxityLevel > 0 & MainMenuScript.JackNPCAnxityLevel > 0)
        {
            NormalEnding.SetActive(true);
        }
        else
        {
            BadEnding.SetActive(true);
        }
    }

    public void RetryButton()
    {
        SceneManager.LoadScene(MainMenu);
    }

    public void QuitButton()
    {
        Application.Quit();
    }
}
