<?php

/**
 * Update PSI author affiliations on one publication.
 */
class change_psi_affiliation extends batch_edit_mods {
    protected $update_dc = false;

    protected function edit_mxds() {
        $csv_rows = isset($this->options['csv_rows']) ? $this->options['csv_rows'] : array();

        if(empty($csv_rows)) return $this->get_mxds_content();

        $edited = false;
        $this->set_dom();
        $this->set_xpath();

        $names = $this->get_xpath()->query('//mods:mods/mods:name[@type="personal"]');

        foreach($names as $name) {
            $author_id = null;
            foreach($name->getElementsByTagName('nameIdentifier') as $identifier) { // getElementsByTagNameNS($this->mxds_namespaceURI,'nameIdentifier')
                if($identifier->getAttribute('type') == 'authorId') {
                    $author_id = $this->normalize_id($identifier->textContent);
                    break;
                }
            }

            if($author_id === null || $author_id === '') continue;
            
            /*
            Affiliation fields
            <name type="personal">
                <namePart type="family">...</namePart>
                <namePart type="given">...</namePart>
                <role>
                    <roleTerm authority="marcrelator" type="text">author</roleTerm>
                </role>
                <alternativeName altType="formal_name">
                    <namePart>...</namePart>
                    <nameIdentifier type="authorId">psi-authors:9640</nameIdentifier>
                </alternativeName>
                <nameIdentifier type="organizational unit id">psi-units:751</nameIdentifier>
                <affiliation type="group">...</affiliation>
                <affiliation type="section"/>
                <affiliation type="department">...</affiliation>
                <affiliation type="division">...</affiliation>
            </name>
            */
            // https://github.com/Lib4RI/lib4ri_pub_db/blob/7.x/utils/classes.inc#L519-L537
            foreach($csv_rows as $row) {
                if(!isset($row['psi_author_id'])) continue;
                if($this->normalize_id($row['psi_author_id']) !== $author_id) continue;

                // Update affiliation fields
                foreach(array('group', 'section', 'department', 'division') as $type) {
                    if(!isset($row[$type])) continue;

                    foreach($name->getElementsByTagName('affiliation') as $affiliation) {
                        if ($affiliation->getAttribute('type') === $type) {
                            $value = trim($row[$type]);

                            if($affiliation->textContent !== $value) {
                                $affiliation->nodeValue = $value;
                                $edited = true;
                            }

                            break;
                        }
                    }
                }

                // Update organisational unit id
                if(isset($row['org_unit_id'])) {
                    foreach($name->getElementsByTagName('nameIdentifier') as $identifier) {
                        if($identifier->getAttribute('type') === 'organizational unit id') {
                            $value = trim($row['org_unit_id']);

                            if($identifier->textContent !== $value) {
                                $identifier->nodeValue = $value;
                                $edited = true;
                            }

                            break;
                        }
                    }
                }

                break;
            }
        }

        return $edited ? $this->get_dom()->saveXML() : $this->get_mxds_content();
    }


    private function normalize_id($id) {
        $id = trim((string) $id);
        $colon = strrpos($id, ':');

        return trim($colon !== false ? substr($id, $colon + 1) : $id);
    }


    /**
     * Update, create or remove a typed XML element.
     */
    /*
    // TODO: refactor version with new fields
    private function set_tagged($parent, $tag, $type, $value) {
        $value = trim((string) $value);
        $node = null;

        foreach ($parent->childNodes as $child) {
            if (
                $child instanceof DOMElement &&
                $child->localName === $tag &&
                $child->namespaceURI === $this->mxds_namespaceURI &&
                $child->getAttribute('type') === $type
            ) {
                $node = $child;
                break;
            }
        }

        // Empty CSV value
        if($value === '') {
            if($node === null) return false;

            $parent->removeChild($node);
            return true;
        }

        if($node !== null) {
            if($node->textContent === $value) return false;

            while($node->firstChild) $node->removeChild($node->firstChild);
            $node->appendChild($this->get_dom()->createTextNode($value));

            return true;
        }

        // Missing element
        $node = $this->get_dom()->createElementNS($this->mxds_namespaceURI,$tag);
        $node->setAttribute('type', $type);

        $node->appendChild($this->get_dom()->createTextNode($value));
        $parent->appendChild($node);

        return true;
    }
    */
}