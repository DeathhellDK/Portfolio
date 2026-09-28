using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;

public class tutorialschool : MonoBehaviour
{
    public GameObject tutorialPanel1;

    public TextMeshProUGUI tutorialSchool;


    // Update is called once per frame
    void Update()
    {
        if(MainMenuScript.tutorialcheck == true)
        {
            tutorialschoolOpen();
            MainMenuScript.tutorialcheck = false;
        }
    }

    void tutorialschoolOpen()
    {
        tutorialPanel1.SetActive(true);
        tutorialSchool.text = "Welcome to your school! Your classmates are having some anxiety issues. Go talk to them and help them out! You need to help them make the correct decisions in order to win the game." + "If you run out of energy, don't forget to visit your home!";
    }

    public void closeTutorialschool()
    {
        tutorialPanel1.SetActive(false);
    }
}
