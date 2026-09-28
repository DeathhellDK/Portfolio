using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class AIManager : MonoBehaviour
{
    public float orignalPositionPoint;
    public float orignalPositionPointZ;
    public float distance = 2;
    public float movingSpeed = 2;


    public int directionPoint;
 

    public bool _hasMovementX;
    public bool _hasMovementZ;

    

    // Start is called before the first frame update
    void Start()
    {
        orignalPositionPoint = transform.position.x;
        orignalPositionPointZ = transform.position.z;

        if (_hasMovementX == true)
        {
            directionPoint = 1;
        }

        if (_hasMovementZ == true)
        {
            directionPoint = 1;
        }

        //if (_hasAnxiety == true)
        //{
        //    AnxietyLevel = Random.Range(50,80);
        //    AnxietyIcon.SetActive(true);
        //}

        //_hasCalcAnxiety = false;

    }

    // Update is called once per frame
    void Update()
    {
        AIMovement();

        //AIAnxiety();

        //AIAnxityCal();

        //AnxityCal();
    }

    public void AIMovement()
    {

        if (_hasMovementX == true)
        {

            Quaternion quaternion1 = Quaternion.Euler(0, -90, 0);
            Quaternion quaternion2 = Quaternion.Euler(0, 90, 0);

            ///condition if the x position is greater than the distance been travel from the origin
            if (transform.position.x > orignalPositionPoint + distance)
            {


                /// AI will turn left
                directionPoint = -1;
                transform.rotation = quaternion1;

            }

            else
            {
                ///condition if the x position is smaller than the distance been travel from the origin
                if (transform.position.x < orignalPositionPoint - distance)
                {

                    /// AI will turn right
                    directionPoint = 1;
                    transform.rotation = quaternion2;
                }
            }

            ///The new position of the AI will be base on the current new postion of the X,Y,Z
            transform.position = new Vector3(transform.localPosition.x + Time.deltaTime * movingSpeed * directionPoint, transform.position.y, transform.position.z);
        }
        else if (_hasMovementZ == true)
        {

            Quaternion quaternion1 = Quaternion.Euler(0, 180, 0);
            Quaternion quaternion2 = Quaternion.Euler(0, 0, 0);
            ///condition if the x position is greater than the distance been travel from the origin
            if (transform.position.z > orignalPositionPointZ + distance)
            {
                /// AI will turn left
                directionPoint = -1;
                transform.rotation = quaternion1;

            }

            else
            {
                ///condition if the x position is smaller than the distance been travel from the origin
                if (transform.position.z < orignalPositionPointZ - distance)
                {
                    /// AI will turn right
                    directionPoint = 1;
                    transform.rotation = quaternion2;
                }
            }

            ///The new position of the AI will be base on the current new postion of the X,Y,Z
            transform.position = new Vector3(transform.position.x, transform.position.y, transform.position.z + Time.deltaTime * movingSpeed * directionPoint);
        }
        else
        {

        }

    }

    private void OnCollisionEnter(Collision collision)
    {
        if (collision.gameObject.tag == "Player")
        {
            movingSpeed = 0;
        }
    }

   
    private void OnCollisionExit(Collision collision)
    {
        if (collision.gameObject.tag == "Player")
        {
            movingSpeed = 2;
        }
    }



}
