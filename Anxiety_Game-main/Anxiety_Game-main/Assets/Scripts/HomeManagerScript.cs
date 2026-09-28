using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using TMPro;

public class HomeManagerScript : MonoBehaviour
{
    public GameObject tutorialPanel;

    public TextMeshProUGUI tutorialText;

    // Update is called once per frame
    void Update()
    {
        HealthBarManager.getHealthBarLife();

        if (MainMenuScript.tutcheck == true)
        {
            tutorialOpen();
            MainMenuScript.tutcheck = false;
        }
    }
    void tutorialOpen()
    {
        tutorialPanel.SetActive(true);
        tutorialText.text = "Welcome to your home! The objective of this game is to help your classmates through their problems and lower their anxiety levels." + "The bed to your left will replenish your energy if you run out. Go through the door to visit your classmates. ";
    }

    public void closeTutorial()
    {
        tutorialPanel.SetActive(false);
    }
}
