using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.SceneManagement;

public class HowToPlayScript : MonoBehaviour
{
    public string MainMenu;
    public string HomeGameScene;

    public void PlayGame()
    {
        SceneManager.LoadScene(HomeGameScene);
    }
    public void BackToMainMenu()
    {
        SceneManager.LoadScene(MainMenu);
    }
}
