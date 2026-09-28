using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class BedNotification : MonoBehaviour
{
    public GameObject NotificationOfHealthRestoration;

    private void OnCollisionEnter(Collision collision)
    {
        if(collision.gameObject.tag == "Player")
        {
            NotificationOfHealthRestoration.SetActive(true);
        }
    }

    public void CloseNotificationOfHealthRestoration()
    {
        NotificationOfHealthRestoration.SetActive(false);
    }
}
