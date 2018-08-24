pipeline {

  agent {
    docker {
      image "gcr.io/organic-storm-201412/fetch-ledger-develop:latest"
    }
  }

  stages {
    stage ('Documentation') {

      when {
        expression {
          return env.GIT_BRANCH == 'PR-191-head'
        } // expression
      } // when

      steps {
        sh 'env'
      } // steps

    } // stage

    stage ('Debug') {
      steps {
        sh 'env'
        sh 'ls -l'
      }
    }
  } // stages
} // pipeline

