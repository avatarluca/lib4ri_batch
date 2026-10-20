<?php

/**
 * Update PSI author affiliations on one publication.
 */
class change_psi_affiliation extends batch_edit_mods {

    protected $update_dc = false;

    protected function edit_mxds() {
        $csv_rows = isset($this->options['csv_rows']) && is_array($this->options['csv_rows']) ? $this->options['csv_rows'] : array();

        if(empty($csv_rows)) return $this->get_mxds_content();

        $rows = array();

        foreach($csv_rows as $row) {
            if(!isset($row['psi_author_id'])) continue;
            
            $author_id = $this->normalize_id($row['psi_author_id']);
            if($author_id === '') continue;

            $rows[$author_id] = $row;
        }

        if(empty($rows)) return $this->get_mxds_content();

        $edited = false;
        $this->set_dom();
        $this->set_xpath();

        $names = $this->get_xpath()->query('//mods:mods/mods:name[@type="personal"]');

        foreach($names as $name) {
            $author_id = null;
            foreach($name->getElementsByTagNameNS($this->mxds_namespaceURI,'nameIdentifier') as $identifier) {
                if($identifier->getAttribute('type') === 'authorId') {
                    $author_id = $this->normalize_id($identifier->textContent);
                    break;
                }
            }

            if($author_id === null || $author_id === '') continue;
            
            if(!isset($rows[$author_id])) continue;
            
            $row = $rows[$author_id];

            foreach(array('group', 'section', 'department', 'division') as $type) {
                if(!array_key_exists($type, $row)) continue;
                if($this->set_tagged($name,'affiliation',$type,$row[$type])) $edited = true;
            }

            if(array_key_exists('org_unit_id', $row)) {
                if($this->set_tagged($name,'nameIdentifier','organizational unit id',$row['org_unit_id'])) $edited = true;
            }
        }

        return $edited ? $this->get_dom()->saveXML() : $this->get_mxds_content();
    }


    /**
     * Change identifiers such as "psi:1234" to "1234" (i.e. remove "psi:").
     */
    private function normalize_id($value) {
        $value = trim((string) $value);
        $colon = strrpos($value, ':');
        if($colon !== false) $value = substr($value, $colon + 1);
        
        return trim($value);
    }


    /**
     * Find direct child elements in the MODS namespace.
     */
    private function get_direct_elements($parent, $tag) {
        $elements = array();
        foreach ($parent->childNodes as $child) {
            if($child instanceof DOMElement && $child->localName === $tag && $child->namespaceURI === $this->mxds_namespaceURI) $elements[] = $child;
        }

        return $elements;
    }


    /**
     * Update, insert or remove a typed MODS element.
     */
    private function set_tagged($parent, $tag, $type, $value) {
        $value = trim((string) $value);
        $matches = array();

        foreach($this->get_direct_elements($parent, $tag) as $node) {
            if($node->getAttribute('type') === $type) $matches[] = $node;
        }

        if($value === '') {
            foreach ($matches as $node) $node->parentNode->removeChild($node);

            return !empty($matches);
        }

        if(!empty($matches)) {
            $node = array_shift($matches);
            $edited = false;

            foreach($matches as $duplicate) {
                $duplicate->parentNode->removeChild($duplicate);
                $edited = true;
            }

            if($node->textContent !== $value) {
                while($node->firstChild) $node->removeChild($node->firstChild);

                $node->appendChild($this->get_dom()->createTextNode($value));

                $edited = true;
            }

            return $edited;
        }

        $prefix = $parent->prefix;

        $qualified_name = ($prefix !== null && $prefix !== '') ? $prefix . ':' . $tag : $tag;

        $node = $this->get_dom()->createElementNS($this->mxds_namespaceURI, $qualified_name);

        $node->setAttribute('type', $type);

        $node->appendChild($this->get_dom()->createTextNode($value));

        if($tag === 'affiliation') {
            $later_elements = array(
                'role',
                'description',
                'etal',
                'nameIdentifier',
            );

            foreach($parent->childNodes as $child) {
                if(
                    $child instanceof DOMElement &&
                    $child->namespaceURI === $this->mxds_namespaceURI &&
                    in_array(
                        $child->localName,
                        $later_elements,
                        true
                    )
                ) {
                    $parent->insertBefore($node, $child);
                    return true;
                }
            }
        }

        $parent->appendChild($node);

        return true;
    }
}