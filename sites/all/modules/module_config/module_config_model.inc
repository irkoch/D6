<?php

class Module_config_model
{

    var $modules_list = array();

    function mc_module_set_status($module)
    {
        db_query('UPDATE {module_config} SET status = 1 WHERE id = %d', $this->modules_list[$module]);
    }

    /*
     * load a list of module config resources by type
     * @param string $type resource type
     * @param int $status resource status
     * @return array $list resources list
     */

    function mc_modules_list($type = 'module', $status = NULL)
    {
        $query = 'SELECT id, name FROM {module_config}';
        if($status)
        {   // add status filter
            $query .= ' WHERE status = %d';
        }

        $result = db_query($query, $status);       

        while($row = db_fetch_array($result))
        { // load module list in array
            $this->modules_list[$row['name']] = $row['id'];
        }
        
    }


    /*
     * Clean data status for disabled resources
     * and remove data for uninstalled resources
     *
     * @param array $resources resources list
     * @param string $type resources type
     *
     */

    function module_config_clean_data($type = 'module')
    {
        foreach($this->modules_list as $rname => $rid)
        {
            $status = db_result(
                db_query(
                    "SELECT status FROM {system} WHERE name = '%s' AND type = '%s'",
                    $rname, $type ));

            if(intval($status))
            { // set the status for resource
                db_query('UPDATE {module_config} SET status = %d WHERE id = %d',
                        0, $rid);
            }
            else
            { // remove resource data from module config
                if(!module_config_clear_data($rid))
                {
                    drupal_set_message('Unsuccessful clear data for '.$rname,
                      'Module Config');
                }
            }
        }
    }


    /*
     * load module config variables for specific resource
     * @param string $name resource name
     * @return array $module_config module config saved variables
     */

     public static function module_config_get_module_config($name, $status = NULL)
     {
        $query = 'SELECT * FROM {module_config} WHERE name = \'%s\' ';
        if(intval($status))
          $query .= ' AND status = %d';

        $module_config = db_fetch_array(db_query($query, $name, $status));
        if(!empty($module_config))
        {
           $query = 'SELECT * FROM {module_config_variable} WHERE scid = %d';
           $result = db_query($query, $module_config['id']);
           while($variable = db_fetch_array($result))
           {
                $variable['value'] = unserialize($variable['value']);
                $module_config['variables'][$variable['name']] = $variable;
           }

           return $module_config;
        }
    }

    function mc_insert_module_config($module, $type, $status)
    {
        $query = "INSERT INTO {module_config} SET name = '%s', type = '%s', status='%d'";
        $values = array(
            'name'      => $module,
            'type'      => $type,
            'status'    => $status ? 1 : 0
        );

        if($values['type'] and $values['status'])
        {
            if(db_query($query, $values)){
                 array_merge(
                          array('id' => db_last_insert_id('module_config', 'id')),
                          $values);
            }
        }
        else{
            drupal_set_message('Module :: '.$module.' XML Errors', 'Module Config');
        }
    }

    function mc_insert_module_variable($scid, $name, $value) {
        $query = 'INSERT INTO {module_config_variable}
                SET scid = %d, name=\'%s\', value = \'%s\' ';
        $values = array(
            'scid'  => $scid,
            'name'  => trim($name),
            'value' => $value? serialize($value) : serialize(''),
        );

        if(db_query($query, $values))
        {
            return $values;
        }
    }

    
    /*
     * Clear or reset data for a specific resource
     *
     * @param int $reource_id
     *
     */

    function module_config_clear_data($resource_id)
    {
        $query = 'DELETE FROM {module_config_variable} WHERE scid = %d ';
        if(intval($resource_id) and db_query($query, $resource_id))
        {
            return db_query('DELETE FROM {module_config} WHERE id = %d',
                    $resource_id);
        }
    }
    
} // class end